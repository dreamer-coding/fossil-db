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

#ifdef _WIN32
#include <direct.h>
#define BC_MKDIR(p) _mkdir(p)
#define BC_PATH_SEP "\\"
#else
#include <sys/stat.h>
#include <unistd.h>
#define BC_MKDIR(p) mkdir(p, 0755)
#define BC_PATH_SEP "/"
#endif

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
    if (needed > out_size)
        return -1;
    snprintf(out, out_size, "%s%s%s", a, BC_PATH_SEP, b);
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

    fread(buf, 1, size, f);
    buf[size] = 0;
    fclose(f);
    return buf;
}

static int bc_entry_path(fossil_bluecrab_db *db, const char *id, char *out, size_t out_size)
{
    size_t needed = strlen(db->root_path) + strlen(BC_PATH_SEP) + strlen("objects") + strlen(BC_PATH_SEP) + strlen(id) + strlen(".fson") + 1;
    if (needed > out_size)
        return -1;
    snprintf(out, out_size, "%s%sobjects%s%s.fson",
             db->root_path, BC_PATH_SEP, BC_PATH_SEP, id);
    out[out_size - 1] = '\0';
    return 0;
}

static int bc_relations_path(fossil_bluecrab_db *db, char *out, size_t out_size)
{
    size_t needed = strlen(db->root_path) + strlen(BC_PATH_SEP) + strlen("relations.fson") + 1;
    if (needed > out_size)
        return -1;
    snprintf(out, out_size, "%s%srelations.fson",
             db->root_path, BC_PATH_SEP);
    out[out_size - 1] = '\0';
    return 0;
}

static int bc_commits_path(fossil_bluecrab_db *db, char *out, size_t out_size)
{
    size_t needed = strlen(db->root_path) + strlen(BC_PATH_SEP) + strlen("commits") + 1;
    if (needed > out_size)
        return -1;
    snprintf(out, out_size, "%s%scommits",
             db->root_path, BC_PATH_SEP);
    out[out_size - 1] = '\0';
    return 0;
}

static int bc_commit_file(fossil_bluecrab_db *db, uint64_t version, char *out, size_t out_size)
{
    char version_buf[32];
    snprintf(version_buf, sizeof(version_buf), "%llu", (unsigned long long)version);
    size_t needed = strlen(db->root_path) + strlen(BC_PATH_SEP) + strlen("commits") + strlen(BC_PATH_SEP) + strlen(version_buf) + strlen(".fson") + 1;
    if (needed > out_size)
        return -1;
    snprintf(out, out_size, "%s%scommits%s%llu.fson",
             db->root_path, BC_PATH_SEP, BC_PATH_SEP, (unsigned long long)version);
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
    return remove(path);
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
    (void)db;
    (void)source_id;
    (void)target_id;

    return 0;
}

int fossil_db_bluecrab_get_relations(
    fossil_bluecrab_db *db,
    const char *id,
    fossil_bluecrab_relation **out_relations,
    size_t *count)
{
    (void)db;
    (void)id;

    *count = 0;
    *out_relations = NULL;

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
    (void)db;
    (void)field;
    (void)value;

    *results = NULL;
    *count = 0;

    return 0;
}

/*
simple fuzzy search scanning objects
*/

int fossil_db_bluecrab_search_fuzzy(
    fossil_bluecrab_db *db,
    const char *query,
    fossil_bluecrab_search_result **results,
    size_t *count)
{
    char objdir[FOSSIL_BLUECRAB_PATH];
    if (bc_join(objdir, sizeof(objdir), db->root_path, "objects") != 0)
        return -1;

    DIR *d = opendir(objdir);
    if (!d)
        return -1;

    fossil_bluecrab_search_result *list = NULL;
    size_t used = 0;

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

        float s = fossil_db_bluecrab_similarity(data, query);

        if (s > 0.1f)
        {
            list = realloc(list, sizeof(*list) * (used + 1));

            strncpy(list[used].id, ent->d_name, FOSSIL_BLUECRAB_MAX_ID - 1);
            list[used].id[FOSSIL_BLUECRAB_MAX_ID - 1] = '\0';
            list[used].score = s;
            strncpy(list[used].snippet, data, 119);
            list[used].snippet[119] = '\0';

            used++;
        }

        free(data);
    }

    closedir(d);

    *results = list;
    *count = used;

    fossil_db_bluecrab_rank_results(list, used);

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
    (void)db;
    (void)version;

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
    bc_join(objdir, db->root_path, "objects");

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
    (void)db;
    (void)backup_path;

    return 0;
}

int fossil_db_bluecrab_compact(fossil_bluecrab_db *db)
{
    return fossil_db_bluecrab_meta_rebuild(db);
}

int fossil_db_bluecrab_verify(fossil_bluecrab_db *db)
{
    return fossil_db_bluecrab_meta_rebuild(db);
}
