/**
 * -----------------------------------------------------------------------------
 * Project: Fossil Logic
 *
 * This file is part of the Fossil Logic project, which aims to develop
 * high-performance, cross-platform applications and libraries. The code
 * contained herein is licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License. You may obtain
 * a copy of the License at:
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
 * License for the specific language governing permissions and limitations
 * under the License.
 *
 * Author: Michael Gene Brockus (Dreamer)
 * Date: 04/05/2014
 *
 * Copyright (C) 2014-2025 Fossil Logic. All rights reserved.
 * -----------------------------------------------------------------------------
 */
#include "fossil/crabdb/bluecrab.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <dirent.h>
#ifndef DT_DIR
#define DT_DIR 4
#endif

#ifdef _WIN32
#include <direct.h>
#define BC_MKDIR(p) _mkdir(p)
#define BC_PATH_SEP "\\"
#else
#include <unistd.h>
#define BC_MKDIR(p) mkdir(p, 0755)
#define BC_PATH_SEP "/"
#endif

#include <sys/stat.h>

/*
------------------------------------------------------------
Internal Helpers
------------------------------------------------------------
*/

static void bc_set_error(fossil_bluecrab_db *db, const char *msg)
{
    if (!db)
        return;
    strncpy(db->last_error, msg, sizeof(db->last_error) - 1);
}

static void bc_timestamp(char *buf, size_t size)
{
    time_t now = time(NULL);
    struct tm *t = gmtime(&now);
    strftime(buf, size, "%Y-%m-%dT%H:%M:%SZ", t);
}

static int bc_join(char *out, size_t out_size, const char *a, const char *b)
{
    size_t needed = strlen(a) + strlen(BC_PATH_SEP) + strlen(b) + 1;
    if (needed >= out_size)
        return -1;
    int n = snprintf(out, out_size, "%s%s%s", a, BC_PATH_SEP, b);
    if (n < 0 || (size_t)n >= out_size)
        return -1;
    out[out_size - 1] = '\0';
    return 0;
}

static int bc_write_file(const char *path, const char *data)
{
    FILE *f = fopen(path, "wb");
    if (!f)
        return -1;

    fwrite(data, 1, strlen(data), f);
    fclose(f);
    return 0;
}

static char *bc_read_file(const char *path)
{
    FILE *f = fopen(path, "rb");
    if (!f)
        return NULL;

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    rewind(f);

    char *buf = malloc(size + 1);
    if (!buf)
    {
        fclose(f);
        return NULL;
    }

    size_t read_bytes = fread(buf, 1, size, f);
    if (read_bytes != (size_t)size)
    {
        free(buf);
        fclose(f);
        return NULL;
    }
    buf[size] = 0;
    fclose(f);
    return buf;
}

static int bc_entry_path(fossil_bluecrab_db *db, const char *id, char *out, size_t out_size)
{
    int n = snprintf(out, out_size, "%s%sobjects%s%s.fson",
                     db->root_path, BC_PATH_SEP, BC_PATH_SEP, id);
    if (n < 0 || (size_t)n >= out_size)
        return -1;
    out[out_size - 1] = '\0';
    return 0;
}

static int bc_relations_path(fossil_bluecrab_db *db, char *out, size_t out_size)
{
    int n = snprintf(out, out_size, "%s%srelations.fson",
                     db->root_path, BC_PATH_SEP);
    if (n < 0 || (size_t)n >= out_size)
        return -1;
    out[out_size - 1] = '\0';
    return 0;
}

static int bc_commits_path(fossil_bluecrab_db *db, char *out, size_t out_size)
{
    int n = snprintf(out, out_size, "%s%scommits",
                     db->root_path, BC_PATH_SEP);
    if (n < 0 || (size_t)n >= out_size)
        return -1;
    out[out_size - 1] = '\0';
    return 0;
}

static int bc_commit_file(fossil_bluecrab_db *db, uint64_t version, char *out, size_t out_size)
{
    char version_buf[32];
    int n = snprintf(version_buf, sizeof(version_buf), "%llu", (unsigned long long)version);
    if (n < 0 || (size_t)n >= sizeof(version_buf))
        return -1;
    n = snprintf(out, out_size, "%s%scommits%s%llu.fson",
                 db->root_path, BC_PATH_SEP, BC_PATH_SEP, (unsigned long long)version);
    if (n < 0 || (size_t)n >= out_size)
        return -1;
    out[out_size - 1] = '\0';
    return 0;
}

/*
------------------------------------------------------------
Database Lifecycle
------------------------------------------------------------
*/

int fossil_db_bluecrab_create(const char *path, const char *name)
{
    char objects[FOSSIL_BLUECRAB_PATH];
    char commits[FOSSIL_BLUECRAB_PATH];
    char relations[FOSSIL_BLUECRAB_PATH];
    char meta[FOSSIL_BLUECRAB_PATH];

    BC_MKDIR(path);

    bc_join(objects, sizeof(objects), path, "objects");
    BC_MKDIR(objects);

    bc_join(commits, sizeof(commits), path, "commits");
    BC_MKDIR(commits);

    bc_join(relations, sizeof(relations), path, "relations.fson");
    bc_join(meta, sizeof(meta), path, "meta.fson");

    bc_write_file(relations, "array: []");

    char meta_buf[512];
    snprintf(meta_buf, sizeof(meta_buf),
             "{ name: cstr: \"%s\", entries: i64: 0, relations: i64: 0 }",
             name);

    return bc_write_file(meta, meta_buf);
}

int fossil_db_bluecrab_open(fossil_bluecrab_db *db, const char *path)
{
    if (!db)
        return -1;

    memset(db, 0, sizeof(*db));

    strncpy(db->root_path, path, FOSSIL_BLUECRAB_PATH - 1);

    snprintf(db->meta_path, FOSSIL_BLUECRAB_PATH, "%s%smeta.fson",
             path, BC_PATH_SEP);

    db->opened = true;

    fossil_db_bluecrab_meta_load(db);

    return 0;
}

int fossil_db_bluecrab_close(fossil_bluecrab_db *db)
{
    if (!db)
        return -1;
    db->opened = false;
    return 0;
}

int fossil_db_bluecrab_delete(const char *path)
{
    // Recursively delete all files and directories under the given path
    DIR *dir = opendir(path);
    if (!dir)
        return remove(path); // Not a directory, try to remove as file

    struct dirent *entry;
    char fullpath[FOSSIL_BLUECRAB_PATH];
    int ret = 0;
    int is_dir = 0; // Declare is_dir here

    while ((entry = readdir(dir)) != NULL)
    {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;

        if (snprintf(fullpath, sizeof(fullpath), "%s%s%s", path, BC_PATH_SEP, entry->d_name) >= (int)sizeof(fullpath))
        {
            ret = -1;
            break;
        }

#ifdef _DIRENT_HAVE_D_TYPE
        is_dir = (entry->d_type == DT_DIR);
#else
        // If d_type is not available, use stat
        struct stat st;
        if (stat(fullpath, &st) == 0)
            is_dir = S_ISDIR(st.st_mode);
        else
            is_dir = 0;
#endif

        if (is_dir)
        {
            if (fossil_db_bluecrab_delete(fullpath) != 0)
                ret = -1;
        }
        else
        {
            if (remove(fullpath) != 0)
                ret = -1;
        }
    }

    closedir(dir);

#ifdef _WIN32
    if (ret == 0)
        ret = _rmdir(path);
#else
    if (ret == 0)
        ret = rmdir(path);
#endif

    return ret;
}

/*
------------------------------------------------------------
CRUD
------------------------------------------------------------
*/

int fossil_db_bluecrab_insert(
    fossil_bluecrab_db *db,
    const char *id,
    const char *fson_data)
{
    char path[FOSSIL_BLUECRAB_PATH];

    if (bc_entry_path(db, id, path, sizeof(path)) != 0)
    {
        bc_set_error(db, "path too long");
        return -1;
    }

    if (bc_write_file(path, fson_data) != 0)
    {
        bc_set_error(db, "write failed");
        return -1;
    }

    db->entry_count++;
    return 0;
}

int fossil_db_bluecrab_get(
    fossil_bluecrab_db *db,
    const char *id,
    char **out_fson)
{
    char path[FOSSIL_BLUECRAB_PATH];

    if (bc_entry_path(db, id, path, sizeof(path)) != 0)
        return -1;

    *out_fson = bc_read_file(path);

    return *out_fson ? 0 : -1;
}

int fossil_db_bluecrab_update(
    fossil_bluecrab_db *db,
    const char *id,
    const char *fson_data)
{
    return fossil_db_bluecrab_insert(db, id, fson_data);
}

int fossil_db_bluecrab_remove(
    fossil_bluecrab_db *db,
    const char *id)
{
    char path[FOSSIL_BLUECRAB_PATH];
    if (bc_entry_path(db, id, path, sizeof(path)) != 0)
        return -1;

    if (remove(path) != 0)
        return -1;

    db->entry_count--;
    return 0;
}

/*
------------------------------------------------------------
Sub Entries
------------------------------------------------------------
*/

int fossil_db_bluecrab_insert_sub(
    fossil_bluecrab_db *db,
    const char *parent_id,
    const char *sub_id,
    const char *fson_data)
{
    char id[FOSSIL_BLUECRAB_MAX_ID * 2];

    snprintf(id, sizeof(id), "%s_%s", parent_id, sub_id);

    return fossil_db_bluecrab_insert(db, id, fson_data);
}

int fossil_db_bluecrab_get_sub(
    fossil_bluecrab_db *db,
    const char *parent_id,
    const char *sub_id,
    char **out_fson)
{
    char id[FOSSIL_BLUECRAB_MAX_ID * 2];

    snprintf(id, sizeof(id), "%s_%s", parent_id, sub_id);

    return fossil_db_bluecrab_get(db, id, out_fson);
}

/*
------------------------------------------------------------
Relations
------------------------------------------------------------
*/

int fossil_db_bluecrab_link(
    fossil_bluecrab_db *db,
    const char *source_id,
    const char *target_id,
    const char *relation)
{
    char path[FOSSIL_BLUECRAB_PATH];
    if (bc_relations_path(db, path, sizeof(path)) != 0)
        return -1;

    FILE *f = fopen(path, "a");

    if (!f)
        return -1;

    char timebuf[32];
    bc_timestamp(timebuf, sizeof(timebuf));

    fprintf(f,
            "\nobject: { source: cstr:\"%s\", target: cstr:\"%s\", type: cstr:\"%s\", time: datetime:\"%s\" }",
            source_id, target_id, relation, timebuf);

    fclose(f);

    db->relation_count++;

    return 0;
}

int fossil_db_bluecrab_unlink(
    fossil_bluecrab_db *db,
    const char *source_id,
    const char *target_id)
{
    char path[FOSSIL_BLUECRAB_PATH];
    if (bc_relations_path(db, path, sizeof(path)) != 0)
        return -1;

    char *relations_data = bc_read_file(path);
    if (!relations_data)
        return -1;

    // Create a temporary file to write filtered relations
    char tmp_path[FOSSIL_BLUECRAB_PATH];
    int n = snprintf(tmp_path, sizeof(tmp_path), "%s.tmp", path);
    if (n < 0 || (size_t)n >= sizeof(tmp_path))
    {
        free(relations_data);
        return -1;
    }

    FILE *tmpf = fopen(tmp_path, "w");
    if (!tmpf)
    {
        free(relations_data);
        return -1;
    }

    // Remove lines matching both source_id and target_id
    char *line = strtok(relations_data, "\n");
    int removed = 0;
    while (line)
    {
        if (strstr(line, source_id) && strstr(line, target_id))
        {
            removed = 1;
            // skip writing this line
        }
        else
        {
            fprintf(tmpf, "%s\n", line);
        }
        line = strtok(NULL, "\n");
    }

    fclose(tmpf);
    free(relations_data);

    // Replace original file if any line was removed
    if (removed)
    {
        remove(path);
        rename(tmp_path, path);
        db->relation_count--;
        return 0;
    }
    else
    {
        remove(tmp_path);
        return -1;
    }
}

int fossil_db_bluecrab_get_relations(
    fossil_bluecrab_db *db,
    const char *id,
    fossil_bluecrab_relation **out_relations,
    size_t *count)
{
    if (!db || !id || !out_relations || !count)
        return -1;

    char path[FOSSIL_BLUECRAB_PATH];
    if (bc_relations_path(db, path, sizeof(path)) != 0)
        return -1;

    char *relations_data = bc_read_file(path);
    if (!relations_data)
        return -1;

    size_t rel_cap = 8;
    size_t rel_count = 0;
    fossil_bluecrab_relation *rels = malloc(rel_cap * sizeof(fossil_bluecrab_relation));
    if (!rels)
    {
        free(relations_data);
        return -1;
    }

    char *line = strtok(relations_data, "\n");
    while (line)
    {
        if (strstr(line, id))
        {
            // crude parse: extract source, target, type, time
            fossil_bluecrab_relation rel = {0};
            char *src = strstr(line, "source: cstr:\"");
            char *tgt = strstr(line, "target: cstr:\"");
            char *typ = strstr(line, "type: cstr:\"");
            char *tim = strstr(line, "time: datetime:\"");
            if (src && tgt && typ && tim)
            {
                src += strlen("source: cstr:\"");
                tgt += strlen("target: cstr:\"");
                typ += strlen("type: cstr:\"");
                tim += strlen("time: datetime:\"");
                char *src_end = strchr(src, '"');
                char *tgt_end = strchr(tgt, '"');
                char *typ_end = strchr(typ, '"');
                char *tim_end = strchr(tim, '"');
                if (src_end && tgt_end && typ_end && tim_end)
                {
                    size_t n;
                    n = src_end - src;
                    if (n >= sizeof(rel.source_id))
                        n = sizeof(rel.source_id) - 1;
                    strncpy(rel.source_id, src, n);
                    rel.source_id[n] = 0;
                    n = tgt_end - tgt;
                    if (n >= sizeof(rel.target_id))
                        n = sizeof(rel.target_id) - 1;
                    strncpy(rel.target_id, tgt, n);
                    rel.target_id[n] = 0;
                    n = typ_end - typ;
                    if (n >= sizeof(rel.relation_type))
                        n = sizeof(rel.relation_type) - 1;
                    strncpy(rel.relation_type, typ, n);
                    rel.relation_type[n] = 0;
                    n = tim_end - tim;
                    if (n >= sizeof(rel.created_at))
                        n = sizeof(rel.created_at) - 1;
                    strncpy(rel.created_at, tim, n);
                    rel.created_at[n] = 0;
                    // Only add if id matches source or target
                    if (strcmp(rel.source_id, id) == 0 || strcmp(rel.target_id, id) == 0)
                    {
                        if (rel_count == rel_cap)
                        {
                            rel_cap *= 2;
                            fossil_bluecrab_relation *tmp = realloc(rels, rel_cap * sizeof(fossil_bluecrab_relation));
                            if (!tmp)
                            {
                                free(rels);
                                free(relations_data);
                                return -1;
                            }
                            rels = tmp;
                        }
                        rels[rel_count++] = rel;
                    }
                }
            }
        }
        line = strtok(NULL, "\n");
    }

    free(relations_data);

    *out_relations = rels;
    *count = rel_count;

    return 0;
}

/*
------------------------------------------------------------
Search
------------------------------------------------------------
*/

int fossil_db_bluecrab_search_exact(
    fossil_bluecrab_db *db,
    const char *field,
    const char *value,
    fossil_bluecrab_search_result **results,
    size_t *count)
{
    if (!db || !field || !value || !results || !count)
        return -1;

    char objdir[FOSSIL_BLUECRAB_PATH];
    if (bc_join(objdir, sizeof(objdir), db->root_path, "objects") != 0)
        return -1;

    DIR *d = opendir(objdir);
    if (!d)
        return -1;

    fossil_bluecrab_search_result *list = NULL;
    size_t used = 0, cap = 16;

    list = malloc(sizeof(*list) * cap);
    if (!list) {
        closedir(d);
        return -1;
    }

    struct dirent *ent;
    while ((ent = readdir(d)))
    {
        if (!strstr(ent->d_name, ".fson"))
            continue;

        char path[FOSSIL_BLUECRAB_PATH];
        if (snprintf(path, sizeof(path), "%s%s%s", objdir, BC_PATH_SEP, ent->d_name) >= (int)sizeof(path))
            continue;

        char *data = bc_read_file(path);
        if (!data)
            continue;

        // Look for 'field: type:"value"' or 'field: type:value'
        char search_pattern[128];
        snprintf(search_pattern, sizeof(search_pattern), "%s:", field);
        char *field_pos = strstr(data, search_pattern);
        int match = 0;
        if (field_pos)
        {
            field_pos += strlen(search_pattern);
            while (*field_pos == ' ' || *field_pos == '\t')
                field_pos++;
            // Accept quoted or unquoted value
            if (*field_pos == 'c' && strncmp(field_pos, "cstr:", 5) == 0) {
                field_pos += 5;
                while (*field_pos == ' ' || *field_pos == '\t')
                    field_pos++;
                if (*field_pos == '"') {
                    field_pos++;
                    size_t vlen = strlen(value);
                    if (strncmp(field_pos, value, vlen) == 0 && field_pos[vlen] == '"')
                        match = 1;
                }
            } else if (*field_pos == '"') {
                field_pos++;
                size_t vlen = strlen(value);
                if (strncmp(field_pos, value, vlen) == 0 && field_pos[vlen] == '"')
                    match = 1;
            } else {
                size_t vlen = strlen(value);
                if (strncmp(field_pos, value, vlen) == 0 &&
                    (field_pos[vlen] == '\0' || field_pos[vlen] == ',' || field_pos[vlen] == ' ' || field_pos[vlen] == '\n'))
                    match = 1;
            }
        }

        if (match)
        {
            if (used == cap) {
                cap *= 2;
                fossil_bluecrab_search_result *tmp = realloc(list, sizeof(*list) * cap);
                if (!tmp) {
                    free(data);
                    free(list);
                    closedir(d);
                    return -1;
                }
                list = tmp;
            }
            strncpy(list[used].id, ent->d_name, FOSSIL_BLUECRAB_MAX_ID - 1);
            list[used].id[FOSSIL_BLUECRAB_MAX_ID - 1] = '\0';
            list[used].score = 1.0f;
            strncpy(list[used].snippet, data, 119);
            list[used].snippet[119] = '\0';
            used++;
        }

        free(data);
    }

    closedir(d);

    *results = list;
    *count = used;

    return 0;
}

/*
------------------------------------------------------------
AI Similarity
------------------------------------------------------------
*/

float fossil_db_bluecrab_similarity(const char *a, const char *b)
{
    if (!a || !b)
        return 0;

    size_t la = strlen(a);
    size_t lb = strlen(b);

    size_t m = la < lb ? la : lb;
    size_t match = 0;

    for (size_t i = 0; i < m; i++)
        if (a[i] == b[i])
            match++;

    return (float)match / (float)m;
}

int fossil_db_bluecrab_rank_results(
    fossil_bluecrab_search_result *results,
    size_t count)
{
    for (size_t i = 0; i < count; i++)
        for (size_t j = i + 1; j < count; j++)
            if (results[j].score > results[i].score)
            {
                fossil_bluecrab_search_result t = results[i];
                results[i] = results[j];
                results[j] = t;
            }

    return 0;
}

/*
------------------------------------------------------------
Hash
------------------------------------------------------------
*/

int fossil_db_bluecrab_hash_entry(
    const char *data,
    char out_hash[FOSSIL_BLUECRAB_HASH_SIZE])
{
    unsigned long h = 5381;
    int c;

    while ((c = *data++))
        h = ((h << 5) + h) + c;

    snprintf(out_hash, FOSSIL_BLUECRAB_HASH_SIZE, "%lx", h);

    return 0;
}

int fossil_db_bluecrab_verify_entry(
    fossil_bluecrab_db *db,
    const char *id)
{
    char *data;

    if (fossil_db_bluecrab_get(db, id, &data) != 0)
        return -1;

    char hash[FOSSIL_BLUECRAB_HASH_SIZE];

    fossil_db_bluecrab_hash_entry(data, hash);

    free(data);

    return 0;
}

/*
------------------------------------------------------------
Git Hybrid
------------------------------------------------------------
*/

int fossil_db_bluecrab_commit(
    fossil_bluecrab_db *db,
    const char *message)
{
    char path[FOSSIL_BLUECRAB_PATH];

    db->last_commit_version++;

    if (bc_commit_file(db, db->last_commit_version, path, sizeof(path)) != 0)
        return -1;

    char timebuf[32];
    bc_timestamp(timebuf, sizeof(timebuf));

    char buf[512];

    snprintf(buf, sizeof(buf),
             "{ version: u64:%llu, message: cstr:\"%s\", time: datetime:\"%s\" }",
             (unsigned long long)db->last_commit_version,
             message, timebuf);

    return bc_write_file(path, buf);
}

int fossil_db_bluecrab_log(fossil_bluecrab_db *db)
{
    char commits[FOSSIL_BLUECRAB_PATH];
    if (bc_commits_path(db, commits, sizeof(commits)) != 0)
        return -1;

    DIR *d = opendir(commits);
    if (!d)
        return -1;

    struct dirent *e;

    while ((e = readdir(d)))
        printf("%s\n", e->d_name);

    closedir(d);

    return 0;
}

int fossil_db_bluecrab_checkout(
    fossil_bluecrab_db *db,
    const char *version)
{
    if (!db || !version)
        return -1;

    char commit_path[FOSSIL_BLUECRAB_PATH];
    if (bc_commit_file(db, strtoull(version, NULL, 10), commit_path, sizeof(commit_path)) != 0)
        return -1;

    char *commit_data = bc_read_file(commit_path);
    if (!commit_data)
        return -1;

    // In a real implementation, you would restore the DB state from the commit snapshot.
    // Here, just print commit info for demonstration.
    printf("Checking out commit: %s\n%s\n", version, commit_data);

    free(commit_data);
    return 0;
}

/*
------------------------------------------------------------
Meta
------------------------------------------------------------
*/

int fossil_db_bluecrab_meta_load(fossil_bluecrab_db *db)
{
    char *meta = bc_read_file(db->meta_path);

    if (!meta)
        return -1;

    free(meta);

    return 0;
}

int fossil_db_bluecrab_meta_save(fossil_bluecrab_db *db)
{
    char buf[256];

    snprintf(buf, sizeof(buf),
             "{ entries: i64:%zu, relations: i64:%zu }",
             db->entry_count, db->relation_count);

    return bc_write_file(db->meta_path, buf);
}

int fossil_db_bluecrab_meta_rebuild(fossil_bluecrab_db *db)
{
    char objdir[FOSSIL_BLUECRAB_PATH];
    bc_join(objdir, sizeof(objdir), db->root_path, "objects");

    DIR *d = opendir(objdir);
    if (!d)
        return -1;

    size_t count = 0;

    struct dirent *e;

    while ((e = readdir(d)))
        if (strstr(e->d_name, ".fson"))
            count++;

    closedir(d);

    db->entry_count = count;

    return fossil_db_bluecrab_meta_save(db);
}

/*
------------------------------------------------------------
Advanced
------------------------------------------------------------
*/

int fossil_db_bluecrab_backup(
    fossil_bluecrab_db *db,
    const char *backup_path)
{
    char cmd[1024];

#ifdef _WIN32
    snprintf(cmd, sizeof(cmd), "xcopy %s %s /E /I", db->root_path, backup_path);
#else
    snprintf(cmd, sizeof(cmd), "cp -r %s %s", db->root_path, backup_path);
#endif

    return system(cmd);
}

int fossil_db_bluecrab_restore(
    fossil_bluecrab_db *db,
    const char *backup_path)
{
#ifdef _WIN32
    char cmd[1024];
    snprintf(cmd, sizeof(cmd), "xcopy %s %s /E /I /Y", backup_path, db->root_path);
    return system(cmd);
#else
    char cmd[1024];
    snprintf(cmd, sizeof(cmd), "cp -r %s/* %s/", backup_path, db->root_path);
    return system(cmd);
#endif
}

int fossil_db_bluecrab_compact(fossil_bluecrab_db *db)
{
    return fossil_db_bluecrab_meta_rebuild(db);
}

int fossil_db_bluecrab_verify(fossil_bluecrab_db *db)
{
    return fossil_db_bluecrab_meta_rebuild(db);
}
