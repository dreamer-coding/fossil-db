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
#ifndef DT_REG
#define DT_REG 8
#endif
#ifndef DT_DIR
#define DT_DIR 4
#endif

#ifdef _WIN32
#include <direct.h>
#define mkdir(path) _mkdir(path)
#else
#include <sys/stat.h>
#endif


/* -----------------------------------------------------------
Internal Helpers
----------------------------------------------------------- */

static int bluecrab_file_exists(const char *path)
{
    FILE *f = fopen(path, "r");
    if (!f) return 0;
    fclose(f);
    return 1;
}

static int bluecrab_write_file(const char *path, const char *data)
{
    FILE *f = fopen(path, "w");
    if (!f) return -1;

    fputs(data, f);
    fclose(f);
    return 0;
}

static char *bluecrab_read_file(const char *path)
{
    FILE *f = fopen(path, "r");
    if (!f) return NULL;

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    rewind(f);

    char *buffer = malloc(size + 1);
    if (!buffer) {
        fclose(f);
        return NULL;
    }
    size_t read_size = fread(buffer, 1, size, f);
    if (read_size != (size_t)size) {
        free(buffer);
        fclose(f);
        return NULL;
    }

    buffer[size] = 0;
    fclose(f);

    return buffer;
}

static void bluecrab_entry_path(
    fossil_bluecrab_db *db,
    const char *id,
    char *out
)
{
    // Use FOSSIL_BLUECRAB_PATH for output buffer size
    snprintf(out,
             FOSSIL_BLUECRAB_PATH,
             "%s/objects/%s.fson",
             db->root_path,
             id);
}


/* -----------------------------------------------------------
Database Lifecycle
----------------------------------------------------------- */

int fossil_db_bluecrab_create(
    const char *path,
    const char *name
)
{
    char buffer[FOSSIL_BLUECRAB_PATH];

    // Create main database directory
#ifdef _WIN32
    mkdir(path);
#else
    mkdir(path, 0777);
#endif

    // Create objects directory
    snprintf(buffer, sizeof(buffer), "%s/objects", path);
#ifdef _WIN32
    mkdir(buffer);
#else
    mkdir(buffer, 0777);
#endif

    // Create commits directory
    snprintf(buffer, sizeof(buffer), "%s/commits", path);
#ifdef _WIN32
    mkdir(buffer);
#else
    mkdir(buffer, 0777);
#endif

    // Write meta.fson file
    snprintf(buffer, sizeof(buffer), "%s/meta.fson", path);

    char meta[512];
    snprintf(meta, sizeof(meta),
        "{\n"
        "  database: cstr: \"%s\",\n"
        "  created: datetime: \"%lld\",\n"
        "  entries: array: []\n"
        "}\n",
        name,
        (long long)time(NULL));

    return bluecrab_write_file(buffer, meta);
}


int fossil_db_bluecrab_open(
    fossil_bluecrab_db *db,
    const char *path
)
{
    if (!db || !path) return -1;

    memset(db, 0, sizeof(*db));

    // Set root_path
    strncpy(db->root_path, path, FOSSIL_BLUECRAB_PATH - 1);
    db->root_path[FOSSIL_BLUECRAB_PATH - 1] = '\0';

    // Set meta_path
    snprintf(
        db->meta_path,
        sizeof(db->meta_path),
        "%s/meta.fson",
        db->root_path
    );

    // Check meta file exists
    if (!bluecrab_file_exists(db->meta_path))
        return -1;

    // Try to read meta file and extract name, entry_count, last_commit_version, etc.
    char *meta = bluecrab_read_file(db->meta_path);
    if (!meta)
        return -1;

    // Simple parsing for name, entry_count, last_commit_version (not robust, just for demo)
    char *name_ptr = strstr(meta, "database: cstr: \"");
    if (name_ptr) {
        name_ptr += strlen("database: cstr: \"");
        char *end = strchr(name_ptr, '"');
        if (end) {
            size_t len = end - name_ptr;
            if (len >= FOSSIL_BLUECRAB_MAX_ID) len = FOSSIL_BLUECRAB_MAX_ID - 1;
            strncpy(db->name, name_ptr, len);
            db->name[len] = '\0';
        }
    }

    // Optionally parse other fields if needed (entry_count, last_commit_version, etc.)

    db->opened = true;

    free(meta);

    return 0;
}


int fossil_db_bluecrab_close(
    fossil_bluecrab_db *db
)
{
    if (!db) return -1;

    db->opened = false;
    db->entry_count = 0;
    db->relation_count = 0;
    db->last_commit_version = 0;
    db->last_commit_hash[0] = '\0';
    db->last_error[0] = '\0';
    db->name[0] = '\0';
    db->root_path[0] = '\0';
    db->meta_path[0] = '\0';
    db->internal_data = NULL;

    return 0;
}

int fossil_db_bluecrab_delete(
    const char *path
)
{
    // Recursively delete the database directory and its contents.
    // Remove all files and subdirectories under 'path', then remove 'path' itself.

    DIR *dir = opendir(path);
    if (!dir)
        return -1;

    struct dirent *entry;
    char buf[FOSSIL_BLUECRAB_PATH * 2];
    int ret = 0;

    while ((entry = readdir(dir))) {
        // Skip "." and ".."
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;

        snprintf(buf, sizeof(buf), "%s/%s", path, entry->d_name);

        if (entry->d_type == DT_DIR) {
            // Recursively delete subdirectory
            if (fossil_db_bluecrab_delete(buf) != 0)
                ret = -2;
        } else {
            // Remove file
            if (remove(buf) != 0)
                ret = -3;
        }
    }
    closedir(dir);

    // Remove the directory itself
#ifdef _WIN32
    if (_rmdir(path) != 0)
        ret = -4;
#else
    if (rmdir(path) != 0)
        ret = -4;
#endif

    return ret;
}


/* -----------------------------------------------------------
CRUD Operations
----------------------------------------------------------- */

int fossil_db_bluecrab_insert(
    fossil_bluecrab_db *db,
    const char *id,
    const char *fson_data
)
{
    if (!db || !db->opened) return -1;

    char path[FOSSIL_BLUECRAB_PATH];
    bluecrab_entry_path(db, id, path);

    if (bluecrab_file_exists(path))
        return -2; // Entry already exists

    // Compute hash of fson_data
    char hash[FOSSIL_BLUECRAB_HASH_SIZE];
    fossil_db_bluecrab_hash_entry(fson_data, hash);

    // Get current time for created_at and updated_at
    time_t now = time(NULL);

    // Compose entry data with metadata fields
    size_t entry_len = strlen(fson_data) + 128;
    char *entry_data = malloc(entry_len);
    if (!entry_data)
        return -3;

    snprintf(entry_data, entry_len,
        "%s\n  created_at: cstr: \"%lld\",\n  updated_at: cstr: \"%lld\",\n  version: u64: %llu,\n  hash: cstr: \"%s\"\n",
        fson_data,
        (long long)now,
        (long long)now,
        (unsigned long long)(db->last_commit_version + 1),
        hash
    );

    int res = bluecrab_write_file(path, entry_data);
    free(entry_data);
    if (res != 0)
        return res;

    // Update entry_count in db struct
    db->entry_count++;

    // Optionally update meta file
    fossil_db_bluecrab_meta_save(db);

    return 0;
}


int fossil_db_bluecrab_get(
    fossil_bluecrab_db *db,
    const char *id,
    char **out_fson
)
{
    if (!db || !db->opened || !out_fson) return -1;

    char path[FOSSIL_BLUECRAB_PATH];
    bluecrab_entry_path(db, id, path);

    *out_fson = bluecrab_read_file(path);

    if (!*out_fson)
        return -1;

    return 0;
}


int fossil_db_bluecrab_update(
    fossil_bluecrab_db *db,
    const char *id,
    const char *fson_data
)
{
    if (!db || !db->opened) return -1;

    char path[FOSSIL_BLUECRAB_PATH];
    bluecrab_entry_path(db, id, path);

    if (!bluecrab_file_exists(path))
        return -2; // Entry does not exist

    // Read the existing entry data
    char *old_data = bluecrab_read_file(path);

    // Prepare new data with updated fields
    char *new_data = NULL;
    size_t new_data_len = 0;

    // Compute new hash
    char hash[FOSSIL_BLUECRAB_HASH_SIZE];
    fossil_db_bluecrab_hash_entry(fson_data, hash);

    // Get current time for updated_at
    time_t now = time(NULL);

    // Try to update updated_at, version, hash fields if present, else append them
    // For simplicity, just append/replace at the end (not robust FSON parsing)
    // You may want to use a real FSON parser for production

    // Compose new data
    new_data_len = strlen(fson_data) + 128;
    new_data = malloc(new_data_len);
    if (!new_data) {
        if (old_data) free(old_data);
        return -3;
    }

    // Remove any existing hash/updated_at/version fields (simple, not robust)
    // For now, just ignore and append new fields
    snprintf(new_data, new_data_len,
        "%s\n  updated_at: cstr: \"%lld\",\n  version: u64: %llu,\n  hash: cstr: \"%s\"\n",
        fson_data,
        (long long)now,
        (unsigned long long)(db->last_commit_version + 1),
        hash
    );

    int res = bluecrab_write_file(path, new_data);

    if (old_data) free(old_data);
    free(new_data);

    // Optionally update db->last_commit_version
    db->last_commit_version++;

    // Optionally update meta file
    fossil_db_bluecrab_meta_save(db);

    return res;
}


int fossil_db_bluecrab_remove(
    fossil_bluecrab_db *db,
    const char *id
)
{
    if (!db || !db->opened) return -1;

    char path[FOSSIL_BLUECRAB_PATH];
    bluecrab_entry_path(db, id, path);

    // Check if entry exists
    if (!bluecrab_file_exists(path))
        return -2; // Entry does not exist

    // Remove the entry file
    if (remove(path) != 0)
        return -3; // Failed to remove

    // Update entry_count in db struct
    if (db->entry_count > 0)
        db->entry_count--;

    fossil_db_bluecrab_meta_save(db);

    return 0;
}


/* -----------------------------------------------------------
Sub Entries
----------------------------------------------------------- */

int fossil_db_bluecrab_insert_sub(
    fossil_bluecrab_db *db,
    const char *parent_id,
    const char *sub_id,
    const char *fson_data
)
{
    if (!db || !db->opened) return -1;

    char id[FOSSIL_BLUECRAB_MAX_ID];
    snprintf(id, sizeof(id), "%s_%s", parent_id, sub_id);

    fossil_bluecrab_entry entry = {0};
    strncpy(entry.id, id, FOSSIL_BLUECRAB_MAX_ID - 1);
    strncpy(entry.parent_id, parent_id, FOSSIL_BLUECRAB_MAX_ID - 1);
    entry.version = db->last_commit_version;
    entry.data_size = fson_data ? strlen(fson_data) : 0;
    entry.fson_data = (char *)fson_data;
    entry.deleted = false;
    entry.flags = 0;

    // Set created_at and updated_at to current time as string
    time_t now = time(NULL);
    snprintf(entry.created_at, sizeof(entry.created_at), "%lld", (long long)now);
    snprintf(entry.updated_at, sizeof(entry.updated_at), "%lld", (long long)now);

    // Compute hash of fson_data
    fossil_db_bluecrab_hash_entry(fson_data, entry.hash);

    // Insert as a normal entry with a composite id
    return fossil_db_bluecrab_insert(db, entry.id, entry.fson_data);
}


int fossil_db_bluecrab_get_sub(
    fossil_bluecrab_db *db,
    const char *parent_id,
    const char *sub_id,
    char **out_fson
)
{
    if (!db || !db->opened || !out_fson) return -1;

    char id[FOSSIL_BLUECRAB_MAX_ID];
    snprintf(id, sizeof(id), "%s_%s", parent_id, sub_id);

    return fossil_db_bluecrab_get(db, id, out_fson);
}


/* -----------------------------------------------------------
Relationships
----------------------------------------------------------- */

int fossil_db_bluecrab_link(
    fossil_bluecrab_db *db,
    const char *source_id,
    const char *target_id,
    const char *relation
)
{
    if (!db || !db->opened) return -1;

    char path[FOSSIL_BLUECRAB_PATH];
    snprintf(path, sizeof(path), "%s/relations.fson", db->root_path);

    FILE *f = fopen(path, "a");
    if (!f) return -1;

    time_t now = time(NULL);

    fprintf(f,
        "{ source: cstr:\"%s\", target: cstr:\"%s\", type: cstr:\"%s\", created_version: u64:%llu, created_at: cstr:\"%lld\", metadata: cstr:\"\" }\n",
        source_id,
        target_id,
        relation,
        (unsigned long long)db->last_commit_version,
        (long long)now
    );

    fclose(f);

    db->relation_count++;

    return 0;
}


int fossil_db_bluecrab_unlink(
    fossil_bluecrab_db *db,
    const char *source_id,
    const char *target_id
)
{
    // Advanced: remove only the relation where both source_id and target_id match exactly.
    if (!db || !db->opened) return -1;

    char path[FOSSIL_BLUECRAB_PATH];
    snprintf(path, sizeof(path), "%s/relations.fson", db->root_path);

    FILE *f_in = fopen(path, "r");
    if (!f_in) return -2;

    // Temporary file to write filtered relations
    char tmp_path[FOSSIL_BLUECRAB_PATH];
    snprintf(tmp_path, sizeof(tmp_path), "%s/relations.tmp", db->root_path);

    FILE *f_out = fopen(tmp_path, "w");
    if (!f_out) {
        fclose(f_in);
        return -3;
    }

    char line[512];
    int removed = 0;
    while (fgets(line, sizeof(line), f_in)) {
        // Parse source and target fields exactly
        char *src = strstr(line, "source: cstr:\"");
        char *tgt = strstr(line, "target: cstr:\"");
        char src_id[FOSSIL_BLUECRAB_MAX_ID] = {0};
        char tgt_id[FOSSIL_BLUECRAB_MAX_ID] = {0};

        if (src) {
            src += strlen("source: cstr:\"");
            char *end = strchr(src, '"');
            if (end) {
                size_t len = end - src;
                if (len >= FOSSIL_BLUECRAB_MAX_ID) len = FOSSIL_BLUECRAB_MAX_ID - 1;
                strncpy(src_id, src, len);
                src_id[len] = '\0';
            }
        }
        if (tgt) {
            tgt += strlen("target: cstr:\"");
            char *end = strchr(tgt, '"');
            if (end) {
                size_t len = end - tgt;
                if (len >= FOSSIL_BLUECRAB_MAX_ID) len = FOSSIL_BLUECRAB_MAX_ID - 1;
                strncpy(tgt_id, tgt, len);
                tgt_id[len] = '\0';
            }
        }
        if (src_id[0] && tgt_id[0] &&
            strcmp(src_id, source_id) == 0 &&
            strcmp(tgt_id, target_id) == 0) {
            // This is the exact relation to remove
            removed++;
            continue;
        }
        fputs(line, f_out);
    }

    fclose(f_in);
    fclose(f_out);

    // Replace original file with filtered file
    remove(path);
    rename(tmp_path, path);

    if (removed > 0 && db->relation_count > 0)
        db->relation_count--;

    return removed > 0 ? 0 : -4;
}


int fossil_db_bluecrab_get_relations(
    fossil_bluecrab_db *db,
    const char *id,
    fossil_bluecrab_relation **out_relations,
    size_t *count
)
{
    if (!db || !db->opened || !out_relations || !count) return -1;

    char path[FOSSIL_BLUECRAB_PATH];
    snprintf(path, sizeof(path), "%s/relations.fson", db->root_path);

    FILE *f = fopen(path, "r");
    if (!f) return -2;

    size_t cap = 8;
    size_t n = 0;
    fossil_bluecrab_relation *rels = malloc(cap * sizeof(fossil_bluecrab_relation));
    if (!rels) {
        fclose(f);
        return -3;
    }

    char line[512];
    while (fgets(line, sizeof(line), f)) {
        // Only match if id is a full source or target, not just substring
        char *src = strstr(line, "source: cstr:\"");
        char *tgt = strstr(line, "target: cstr:\"");
        int match = 0;
        char src_id[FOSSIL_BLUECRAB_MAX_ID] = {0};
        char tgt_id[FOSSIL_BLUECRAB_MAX_ID] = {0};

        if (src) {
            src += strlen("source: cstr:\"");
            char *end = strchr(src, '"');
            if (end) {
                size_t len = end - src;
                if (len >= FOSSIL_BLUECRAB_MAX_ID) len = FOSSIL_BLUECRAB_MAX_ID - 1;
                strncpy(src_id, src, len);
                src_id[len] = '\0';
                if (strcmp(src_id, id) == 0)
                    match = 1;
            }
        }
        if (tgt) {
            tgt += strlen("target: cstr:\"");
            char *end = strchr(tgt, '"');
            if (end) {
                size_t len = end - tgt;
                if (len >= FOSSIL_BLUECRAB_MAX_ID) len = FOSSIL_BLUECRAB_MAX_ID - 1;
                strncpy(tgt_id, tgt, len);
                tgt_id[len] = '\0';
                if (strcmp(tgt_id, id) == 0)
                    match = 1;
            }
        }

        if (match) {
            fossil_bluecrab_relation rel = {0};
            strncpy(rel.source_id, src_id, FOSSIL_BLUECRAB_MAX_ID - 1);
            strncpy(rel.target_id, tgt_id, FOSSIL_BLUECRAB_MAX_ID - 1);

            // Parse relation_type
            char *typ = strstr(line, "type: cstr:\"");
            if (typ) {
                typ += strlen("type: cstr:\"");
                char *end = strchr(typ, '"');
                if (end) {
                    size_t len = end - typ;
                    if (len >= FOSSIL_BLUECRAB_MAX_ID) len = FOSSIL_BLUECRAB_MAX_ID - 1;
                    strncpy(rel.relation_type, typ, len);
                    rel.relation_type[len] = '\0';
                }
            }
            // Parse created_version
            char *ver = strstr(line, "created_version: u64:");
            if (ver) {
                ver += strlen("created_version: u64:");
                rel.created_version = strtoull(ver, NULL, 10);
            }
            // Parse created_at
            char *cat = strstr(line, "created_at: cstr:\"");
            if (cat) {
                cat += strlen("created_at: cstr:\"");
                char *end = strchr(cat, '"');
                if (end) {
                    size_t len = end - cat;
                    if (len >= sizeof(rel.created_at)) len = sizeof(rel.created_at) - 1;
                    strncpy(rel.created_at, cat, len);
                    rel.created_at[len] = '\0';
                }
            }
            // Parse metadata
            char *meta = strstr(line, "metadata: cstr:\"");
            if (meta) {
                meta += strlen("metadata: cstr:\"");
                char *end = strchr(meta, '"');
                if (end) {
                    size_t len = end - meta;
                    if (len >= sizeof(rel.metadata)) len = sizeof(rel.metadata) - 1;
                    strncpy(rel.metadata, meta, len);
                    rel.metadata[len] = '\0';
                }
            }

            if (n >= cap) {
                cap *= 2;
                fossil_bluecrab_relation *tmp = realloc(rels, cap * sizeof(fossil_bluecrab_relation));
                if (!tmp) {
                    free(rels);
                    fclose(f);
                    return -4;
                }
                rels = tmp;
            }
            rels[n++] = rel;
        }
    }

    fclose(f);

    *out_relations = rels;
    *count = n;

    return 0;
}


/* -----------------------------------------------------------
Search
----------------------------------------------------------- */

int fossil_db_bluecrab_search_exact(
    fossil_bluecrab_db *db,
    const char *field,
    const char *value,
    fossil_bluecrab_search_result **results,
    size_t *count
)
{
    if (!db || !db->opened || !field || !value || !results || !count)
        return -1;

    char objects_dir[FOSSIL_BLUECRAB_PATH];
    snprintf(objects_dir, sizeof(objects_dir), "%s/objects", db->root_path);

    DIR *dir = opendir(objects_dir);
    if (!dir)
        return -2;

    size_t cap = 8, n = 0;
    fossil_bluecrab_search_result *res = malloc(cap * sizeof(fossil_bluecrab_search_result));
    if (!res) {
        closedir(dir);
        return -3;
    }

    struct dirent *entry;
    while ((entry = readdir(dir))) {
        if (entry->d_type != DT_REG)
            continue;
        const char *dot = strrchr(entry->d_name, '.');
        if (!dot || strcmp(dot, ".fson") != 0)
            continue;

        char path[FOSSIL_BLUECRAB_PATH];
        snprintf(path, sizeof(path), "%s/%s", objects_dir, entry->d_name);

        char *data = bluecrab_read_file(path);
        if (!data)
            continue;

        // Advanced: search for field: cstr: "value" or field: ... value (for other types)
        char search_cstr[128];
        snprintf(search_cstr, sizeof(search_cstr), "%s: cstr: \"%s\"", field, value);
        char *found = strstr(data, search_cstr);

        // If not found, try as integer/float/other (e.g., field: u64: value)
        if (!found) {
            char search_num[128];
            snprintf(search_num, sizeof(search_num), "%s: u64: %s", field, value);
            found = strstr(data, search_num);
            if (!found) {
                snprintf(search_num, sizeof(search_num), "%s: i64: %s", field, value);
                found = strstr(data, search_num);
            }
            if (!found) {
                snprintf(search_num, sizeof(search_num), "%s: f64: %s", field, value);
                found = strstr(data, search_num);
            }
        }

        // If not found, try as boolean (true/false)
        if (!found) {
            char search_bool[128];
            snprintf(search_bool, sizeof(search_bool), "%s: bool: %s", field, value);
            found = strstr(data, search_bool);
        }

        // If not found, try as enum (field: enum: value)
        if (!found) {
            char search_enum[128];
            snprintf(search_enum, sizeof(search_enum), "%s: enum: %s", field, value);
            found = strstr(data, search_enum);
        }

        if (found) {
            if (n >= cap) {
                cap *= 2;
                fossil_bluecrab_search_result *tmp = realloc(res, cap * sizeof(fossil_bluecrab_search_result));
                if (!tmp) {
                    free(data);
                    free(res);
                    closedir(dir);
                    return -4;
                }
                res = tmp;
            }
            memset(&res[n], 0, sizeof(fossil_bluecrab_search_result));
            strncpy(res[n].id, entry->d_name, FOSSIL_BLUECRAB_MAX_ID - 1);
            strncpy(res[n].matched_field, field, FOSSIL_BLUECRAB_MAX_ID - 1);
            res[n].score = 1.0f;
            // Copy a snippet (first 120 chars after match)
            size_t offset = found - data;
            size_t sniplen = strlen(data + offset);
            if (sniplen > 127) sniplen = 127;
            strncpy(res[n].snippet, data + offset, sniplen);
            res[n].snippet[sniplen] = '\0';
            n++;
        }
        free(data);
    }
    closedir(dir);

    *results = res;
    *count = n;
    return 0;
}


int fossil_db_bluecrab_search_fuzzy(
    fossil_bluecrab_db *db,
    const char *query,
    fossil_bluecrab_search_result **results,
    size_t *count
)
{
    if (!db || !db->opened || !query || !results || !count)
        return -1;

    char objects_dir[FOSSIL_BLUECRAB_PATH];
    snprintf(objects_dir, sizeof(objects_dir), "%s/objects", db->root_path);

    DIR *dir = opendir(objects_dir);
    if (!dir)
        return -2;

    size_t cap = 8, n = 0;
    fossil_bluecrab_search_result *res = malloc(cap * sizeof(fossil_bluecrab_search_result));
    if (!res) {
        closedir(dir);
        return -3;
    }

    struct dirent *entry;
    while ((entry = readdir(dir))) {
        if (entry->d_type != DT_REG)
            continue;
        const char *dot = strrchr(entry->d_name, '.');
        if (!dot || strcmp(dot, ".fson") != 0)
            continue;

        char path[FOSSIL_BLUECRAB_PATH];
        snprintf(path, sizeof(path), "%s/%s", objects_dir, entry->d_name);

        char *data = bluecrab_read_file(path);
        if (!data)
            continue;

        float score = fossil_db_bluecrab_similarity(data, query);
        if (score > 0.0f) {
            if (n >= cap) {
                cap *= 2;
                fossil_bluecrab_search_result *tmp = realloc(res, cap * sizeof(fossil_bluecrab_search_result));
                if (!tmp) {
                    free(data);
                    free(res);
                    closedir(dir);
                    return -4;
                }
                res = tmp;
            }
            memset(&res[n], 0, sizeof(fossil_bluecrab_search_result));
            strncpy(res[n].id, entry->d_name, FOSSIL_BLUECRAB_MAX_ID - 1);
            res[n].score = score;
            strncpy(res[n].matched_field, "fson_data", FOSSIL_BLUECRAB_MAX_ID - 1);
            // Copy a snippet (first 120 chars of data)
            size_t sniplen = strlen(data);
            if (sniplen > 127) sniplen = 127;
            strncpy(res[n].snippet, data, sniplen);
            res[n].snippet[sniplen] = '\0';
            n++;
        }
        free(data);
    }
    closedir(dir);

    *results = res;
    *count = n;
    return 0;
}


/* -----------------------------------------------------------
AI Similarity
----------------------------------------------------------- */

float fossil_db_bluecrab_similarity(
    const char *a,
    const char *b
)
{
    // Advanced similarity: Levenshtein distance normalized to [0,1]
    size_t la = strlen(a);
    size_t lb = strlen(b);

    if (la == 0 && lb == 0)
        return 1.0f;
    if (la == 0 || lb == 0)
        return 0.0f;

    // Allocate distance matrix
    size_t *prev = malloc((lb + 1) * sizeof(size_t));
    size_t *curr = malloc((lb + 1) * sizeof(size_t));
    if (!prev || !curr) {
        if (prev) free(prev);
        if (curr) free(curr);
        return 0.0f;
    }

    for (size_t j = 0; j <= lb; j++)
        prev[j] = j;

    for (size_t i = 1; i <= la; i++) {
        curr[0] = i;
        for (size_t j = 1; j <= lb; j++) {
            size_t cost = (a[i - 1] == b[j - 1]) ? 0 : 1;
            size_t del = prev[j] + 1;
            size_t ins = curr[j - 1] + 1;
            size_t sub = prev[j - 1] + cost;
            size_t min = del < ins ? del : ins;
            curr[j] = min < sub ? min : sub;
        }
        // Swap prev and curr
        size_t *tmp = prev;
        prev = curr;
        curr = tmp;
    }

    size_t dist = prev[lb];
    free(prev);
    free(curr);

    // Similarity: 1 - normalized distance
    size_t maxlen = la > lb ? la : lb;
    return 1.0f - ((float)dist / (float)maxlen);
}

int fossil_db_bluecrab_rank_results(
    fossil_bluecrab_search_result *results,
    size_t count
)
{
    // Sort results in-place by descending score
    for (size_t i = 0; i < count; i++)
    {
        for (size_t j = i + 1; j < count; j++)
        {
            if (results[j].score > results[i].score)
            {
                fossil_bluecrab_search_result tmp = results[i];
                results[i] = results[j];
                results[j] = tmp;
            }
        }
    }
    return 0;
}


/* -----------------------------------------------------------
Hash
----------------------------------------------------------- */

int fossil_db_bluecrab_hash_entry(
    const char *data,
    char out_hash[FOSSIL_BLUECRAB_HASH_SIZE]
)
{
    // Simple DJB2 hash for demonstration
    unsigned long hash = 5381;
    int c;
    while ((c = *data++))
        hash = ((hash << 5) + hash) + c;
    snprintf(out_hash, FOSSIL_BLUECRAB_HASH_SIZE, "%lx", hash);
    return 0;
}

int fossil_db_bluecrab_verify_entry(
    fossil_bluecrab_db *db,
    const char *id
)
{
    if (!db || !db->opened || !id)
        return -1;

    char *data = NULL;
    if (fossil_db_bluecrab_get(db, id, &data) != 0)
        return -2;

    char computed_hash[FOSSIL_BLUECRAB_HASH_SIZE];
    fossil_db_bluecrab_hash_entry(data, computed_hash);

    // Try to find stored hash in the data (assumes field: hash: cstr: "...")
    char *hash_ptr = strstr(data, "hash: cstr: \"");
    if (hash_ptr) {
        hash_ptr += strlen("hash: cstr: \"");
        char *end = strchr(hash_ptr, '"');
        if (end) {
            char stored_hash[FOSSIL_BLUECRAB_HASH_SIZE] = {0};
            size_t len = end - hash_ptr;
            if (len >= FOSSIL_BLUECRAB_HASH_SIZE) len = FOSSIL_BLUECRAB_HASH_SIZE - 1;
            strncpy(stored_hash, hash_ptr, len);
            stored_hash[len] = '\0';

            int result = strcmp(computed_hash, stored_hash);
            free(data);
            return result == 0 ? 0 : -3; // 0 if match, -3 if mismatch
        }
    }

    free(data);
    return -4; // No stored hash found
    return 0;
}


/* -----------------------------------------------------------
Git Hybrid
----------------------------------------------------------- */

int fossil_db_bluecrab_commit(
    fossil_bluecrab_db *db,
    const char *message
)
{
    if (!db || !db->opened)
        return -1;

    char path[FOSSIL_BLUECRAB_PATH];
    time_t now = time(NULL);
    uint64_t version = db->last_commit_version + 1;

    snprintf(path,
             sizeof(path),
             "%s/commits/%llu.fson",
             db->root_path,
             (unsigned long long)version);

    char commit[512];
    snprintf(commit,
        sizeof(commit),
        "{ message:cstr:\"%s\", version:u64:%llu, time:u64:%lld, hash:cstr:\"%s\" }\n",
        message,
        (unsigned long long)version,
        (long long)now,
        db->last_commit_hash);

    int res = bluecrab_write_file(path, commit);
    if (res == 0) {
        db->last_commit_version = version;
    }
    return res;
}


int fossil_db_bluecrab_log(
    fossil_bluecrab_db *db
)
{
    if (!db || !db->opened)
        return -1;

    char commits_dir[FOSSIL_BLUECRAB_PATH];
    snprintf(commits_dir, sizeof(commits_dir), "%s/commits", db->root_path);

    DIR *dir = opendir(commits_dir);
    if (!dir)
        return -2;

    struct dirent *entry;
    printf("Commit Log for database: %s\n", db->name);
    printf("----------------------------------------\n");
    while ((entry = readdir(dir))) {
        if (entry->d_type != DT_REG)
            continue;
        const char *dot = strrchr(entry->d_name, '.');
        if (!dot || strcmp(dot, ".fson") != 0)
            continue;

        char path[FOSSIL_BLUECRAB_PATH];
        snprintf(path, sizeof(path), "%s/%s", commits_dir, entry->d_name);

        char *data = bluecrab_read_file(path);
        if (data) {
            // Print commit file name and message (simple parse)
            char *msg = strstr(data, "message:cstr:\"");
            if (msg) {
                msg += strlen("message:cstr:\"");
                char *end = strchr(msg, '"');
                if (end) {
                    size_t len = end - msg;
                    char message[128] = {0};
                    if (len > 127) len = 127;
                    strncpy(message, msg, len);
                    message[len] = '\0';
                    printf("Commit: %s\n  Message: %s\n", entry->d_name, message);
                } else {
                    printf("Commit: %s\n", entry->d_name);
                }
            } else {
                printf("Commit: %s\n", entry->d_name);
            }
            free(data);
        }
    }
    closedir(dir);
    printf("----------------------------------------\n");
    return 0;
}

int fossil_db_bluecrab_checkout(
    fossil_bluecrab_db *db,
    const char *version
)
{
    if (!db || !db->opened || !version)
        return -1;

    char commit_path[FOSSIL_BLUECRAB_PATH];
    snprintf(commit_path, sizeof(commit_path), "%s/commits/%s.fson", db->root_path, version);

    if (!bluecrab_file_exists(commit_path))
        return -2;

    // Read commit file to get version/hash (and optionally, snapshot info)
    char *data = bluecrab_read_file(commit_path);
    if (!data)
        return -3;

    // Parse version and hash
    char *ver_ptr = strstr(data, "version:u64:");
    if (ver_ptr) {
        ver_ptr += strlen("version:u64:");
        db->last_commit_version = strtoull(ver_ptr, NULL, 10);
    }
    char *hash_ptr = strstr(data, "hash:cstr:\"");
    if (hash_ptr) {
        hash_ptr += strlen("hash:cstr:\"");
        char *end = strchr(hash_ptr, '"');
        if (end) {
            size_t len = end - hash_ptr;
            if (len >= FOSSIL_BLUECRAB_HASH_SIZE) len = FOSSIL_BLUECRAB_HASH_SIZE - 1;
            strncpy(db->last_commit_hash, hash_ptr, len);
            db->last_commit_hash[len] = '\0';
        }
    }
    free(data);

    // --- Actual file restoration ---
    // Remove all files in objects directory
    char objects_dir[FOSSIL_BLUECRAB_PATH];
    snprintf(objects_dir, sizeof(objects_dir), "%s/objects", db->root_path);

    DIR *dir = opendir(objects_dir);
    if (dir) {
        struct dirent *entry;
        while ((entry = readdir(dir))) {
            if (entry->d_type == DT_REG) {
                char file_path[FOSSIL_BLUECRAB_PATH];
                snprintf(file_path, sizeof(file_path), "%s/%s", objects_dir, entry->d_name);
                remove(file_path);
            }
        }
        closedir(dir);
    }

    // Restore objects from commit snapshot: for each object listed in the commit, restore its content
    // This assumes each commit stores a snapshot of all objects at that version in a directory
    char snapshot_dir[FOSSIL_BLUECRAB_PATH];
    snprintf(snapshot_dir, sizeof(snapshot_dir), "%s/snapshots/%s", db->root_path, version);

    struct stat st;
    if (stat(snapshot_dir, &st) == 0 && S_ISDIR(st.st_mode)) {
        DIR *snapdir = opendir(snapshot_dir);
        if (snapdir) {
            struct dirent *entry;
            while ((entry = readdir(snapdir))) {
                if (entry->d_type == DT_REG) {
                    char src[FOSSIL_BLUECRAB_PATH];
                    char dst[FOSSIL_BLUECRAB_PATH];
                    snprintf(src, sizeof(src), "%s/%s", snapshot_dir, entry->d_name);
                    snprintf(dst, sizeof(dst), "%s/%s", objects_dir, entry->d_name);

                    FILE *fin = fopen(src, "rb");
                    FILE *fout = fopen(dst, "wb");
                    if (fin && fout) {
                        char buf[4096];
                        size_t n;
                        while ((n = fread(buf, 1, sizeof(buf), fin)) > 0)
                            fwrite(buf, 1, n, fout);
                    }
                    if (fin) fclose(fin);
                    if (fout) fclose(fout);
                }
            }
            closedir(snapdir);
        }
    } else {
        // No snapshot directory for this commit version; nothing to restore
        // Optionally, return error or warning
        // return -4;
    }

    // Optionally reload meta
    fossil_db_bluecrab_meta_rebuild(db);

    return 0;
}


/* -----------------------------------------------------------
Meta
----------------------------------------------------------- */

int fossil_db_bluecrab_meta_load(
    fossil_bluecrab_db *db
)
{
    if (!db || !db->meta_path[0])
        return -1;

    char *meta = bluecrab_read_file(db->meta_path);
    if (!meta)
        return -1;

    // Parse name
    char *name_ptr = strstr(meta, "database: cstr: \"");
    if (name_ptr) {
        name_ptr += strlen("database: cstr: \"");
        char *end = strchr(name_ptr, '"');
        if (end) {
            size_t len = end - name_ptr;
            if (len >= FOSSIL_BLUECRAB_MAX_ID) len = FOSSIL_BLUECRAB_MAX_ID - 1;
            strncpy(db->name, name_ptr, len);
            db->name[len] = '\0';
        }
    }

    // Parse entry_count (count entries in entries: array: [ ... ])
    char *entries_ptr = strstr(meta, "entries: array: [");
    if (entries_ptr) {
        entries_ptr += strlen("entries: array: [");
        size_t count = 0;
        char *p = entries_ptr;
        while ((p = strchr(p, '"'))) {
            p++; // skip opening quote
            char *end = strchr(p, '"');
            if (!end) break;
            count++;
            p = end + 1;
        }
        db->entry_count = count;
    }

    // Optionally parse last_commit_version if present
    char *ver_ptr = strstr(meta, "last_commit_version: u64:");
    if (ver_ptr) {
        ver_ptr += strlen("last_commit_version: u64:");
        db->last_commit_version = strtoull(ver_ptr, NULL, 10);
    }

    free(meta);
    return 0;
}

int fossil_db_bluecrab_meta_save(
    fossil_bluecrab_db *db
)
{
    if (!db || !db->meta_path[0])
        return -1;

    // For now, only write name, created, entry_count, last_commit_version
    char meta[1024];
    snprintf(meta, sizeof(meta),
        "{\n"
        "  database: cstr: \"%s\",\n"
        "  created: datetime: \"%lld\",\n"
        "  entry_count: u64: %zu,\n"
        "  last_commit_version: u64: %llu,\n"
        "  entries: array: []\n"
        "}\n",
        db->name,
        (long long)time(NULL),
        db->entry_count,
        (unsigned long long)db->last_commit_version
    );

    return bluecrab_write_file(db->meta_path, meta);
}

int fossil_db_bluecrab_meta_rebuild(
    fossil_bluecrab_db *db
)
{
    if (!db || !db->opened)
        return -1;

    // Recount entries in objects directory
    char objects_dir[FOSSIL_BLUECRAB_PATH];
    snprintf(objects_dir, sizeof(objects_dir), "%s/objects", db->root_path);

    size_t entry_count = 0;
    DIR *dir = opendir(objects_dir);
    if (dir) {
        struct dirent *entry;
        while ((entry = readdir(dir))) {
            if (entry->d_type == DT_REG) {
                const char *dot = strrchr(entry->d_name, '.');
                if (dot && strcmp(dot, ".fson") == 0)
                    entry_count++;
            }
        }
        closedir(dir);
    }
    db->entry_count = entry_count;

    // Recount relations
    char rel_path[FOSSIL_BLUECRAB_PATH];
    snprintf(rel_path, sizeof(rel_path), "%s/relations.fson", db->root_path);
    size_t relation_count = 0;
    FILE *f = fopen(rel_path, "r");
    if (f) {
        char line[512];
        while (fgets(line, sizeof(line), f))
            relation_count++;
        fclose(f);
    }
    db->relation_count = relation_count;

    // Optionally, update last_commit_version by scanning commits directory
    char commits_dir[FOSSIL_BLUECRAB_PATH];
    snprintf(commits_dir, sizeof(commits_dir), "%s/commits", db->root_path);
    uint64_t max_version = 0;
    DIR *cdir = opendir(commits_dir);
    if (cdir) {
        struct dirent *entry;
        while ((entry = readdir(cdir))) {
            if (entry->d_type == DT_REG) {
                const char *dot = strrchr(entry->d_name, '.');
                if (dot && strcmp(dot, ".fson") == 0) {
                    char ver_str[32] = {0};
                    size_t len = dot - entry->d_name;
                    if (len < sizeof(ver_str)) {
                        strncpy(ver_str, entry->d_name, len);
                        uint64_t v = strtoull(ver_str, NULL, 10);
                        if (v > max_version)
                            max_version = v;
                    }
                }
            }
        }
        closedir(cdir);
    }
    db->last_commit_version = max_version;

    // Save meta file
    return fossil_db_bluecrab_meta_save(db);
}


/* -----------------------------------------------------------
Maintenance
----------------------------------------------------------- */

int fossil_db_bluecrab_backup(
    fossil_bluecrab_db *db,
    const char *backup_path
)
{
    if (!db || !db->opened || !backup_path)
        return -1;

    // Create backup directory
#ifdef _WIN32
    mkdir(backup_path);
#else
    mkdir(backup_path, 0777);
#endif

    // List of subdirectories/files to copy
    const char *subdirs[] = { "objects", "commits", "relations.fson", "meta.fson" };
    char src[FOSSIL_BLUECRAB_PATH];
    char dst[FOSSIL_BLUECRAB_PATH];

    for (size_t i = 0; i < 4; ++i) {
        snprintf(src, sizeof(src), "%s/%s", db->root_path, subdirs[i]);
        snprintf(dst, sizeof(dst), "%s/%s", backup_path, subdirs[i]);

        struct stat st;
        if (stat(src, &st) == 0) {
            if (S_ISDIR(st.st_mode)) {
                // Copy directory recursively
                DIR *dir = opendir(src);
                if (!dir) continue;
#ifdef _WIN32
                mkdir(dst);
#else
                mkdir(dst, 0777);
#endif
                struct dirent *entry;
                while ((entry = readdir(dir))) {
                    if (entry->d_type != DT_REG)
                        continue;
                    char file_src[FOSSIL_BLUECRAB_PATH];
                    char file_dst[FOSSIL_BLUECRAB_PATH];
                    snprintf(file_src, sizeof(file_src), "%s/%s", src, entry->d_name);
                    snprintf(file_dst, sizeof(file_dst), "%s/%s", dst, entry->d_name);

                    FILE *fin = fopen(file_src, "rb");
                    FILE *fout = fopen(file_dst, "wb");
                    if (fin && fout) {
                        char buf[4096];
                        size_t n;
                        while ((n = fread(buf, 1, sizeof(buf), fin)) > 0)
                            fwrite(buf, 1, n, fout);
                    }
                    if (fin) fclose(fin);
                    if (fout) fclose(fout);
                }
                closedir(dir);
            } else if (S_ISREG(st.st_mode)) {
                // Copy file
                FILE *fin = fopen(src, "rb");
                FILE *fout = fopen(dst, "wb");
                if (fin && fout) {
                    char buf[4096];
                    size_t n;
                    while ((n = fread(buf, 1, sizeof(buf), fin)) > 0)
                        fwrite(buf, 1, n, fout);
                }
                if (fin) fclose(fin);
                if (fout) fclose(fout);
            }
        }
    }
    return 0;
}


int fossil_db_bluecrab_restore(
    fossil_bluecrab_db *db,
    const char *backup_path
)
{
    if (!db || !backup_path)
        return -1;

    // Restore backup by copying files from backup_path to db->root_path
    const char *subdirs[] = { "objects", "commits", "relations.fson", "meta.fson" };
    char src[FOSSIL_BLUECRAB_PATH];
    char dst[FOSSIL_BLUECRAB_PATH];

    for (size_t i = 0; i < 4; ++i) {
        snprintf(src, sizeof(src), "%s/%s", backup_path, subdirs[i]);
        snprintf(dst, sizeof(dst), "%s/%s", db->root_path, subdirs[i]);

        struct stat st;
        if (stat(src, &st) == 0) {
            if (S_ISDIR(st.st_mode)) {
#ifdef _WIN32
                mkdir(dst);
#else
                mkdir(dst, 0777);
#endif
                DIR *dir = opendir(src);
                if (!dir) continue;
                struct dirent *entry;
                while ((entry = readdir(dir))) {
                    if (entry->d_type != DT_REG)
                        continue;
                    char file_src[FOSSIL_BLUECRAB_PATH];
                    char file_dst[FOSSIL_BLUECRAB_PATH];
                    snprintf(file_src, sizeof(file_src), "%s/%s", src, entry->d_name);
                    snprintf(file_dst, sizeof(file_dst), "%s/%s", dst, entry->d_name);

                    FILE *fin = fopen(file_src, "rb");
                    FILE *fout = fopen(file_dst, "wb");
                    if (fin && fout) {
                        char buf[4096];
                        size_t n;
                        while ((n = fread(buf, 1, sizeof(buf), fin)) > 0)
                            fwrite(buf, 1, n, fout);
                    }
                    if (fin) fclose(fin);
                    if (fout) fclose(fout);
                }
                closedir(dir);
            } else if (S_ISREG(st.st_mode)) {
                FILE *fin = fopen(src, "rb");
                FILE *fout = fopen(dst, "wb");
                if (fin && fout) {
                    char buf[4096];
                    size_t n;
                    while ((n = fread(buf, 1, sizeof(buf), fin)) > 0)
                        fwrite(buf, 1, n, fout);
                }
                if (fin) fclose(fin);
                if (fout) fclose(fout);
            }
        }
    }
    // Optionally reload meta
    if (db->opened)
        fossil_db_bluecrab_meta_load(db);

    return 0;
}


int fossil_db_bluecrab_compact(
    fossil_bluecrab_db *db
)
{
    if (!db || !db->opened)
        return -1;

    // For this file-based DB, compact could mean removing deleted/old files.
    // Not implemented: would require tracking deleted/obsolete entries.
    // Placeholder: just return success.
    return 0;
}


int fossil_db_bluecrab_verify(
    fossil_bluecrab_db *db
)
{
    if (!db || !db->opened)
        return -1;

    // Verify all entries' hashes
    char objects_dir[FOSSIL_BLUECRAB_PATH];
    snprintf(objects_dir, sizeof(objects_dir), "%s/objects", db->root_path);

    DIR *dir = opendir(objects_dir);
    if (!dir)
        return -2;

    struct dirent *entry;
    int errors = 0;
    while ((entry = readdir(dir))) {
        if (entry->d_type != DT_REG)
            continue;
        const char *dot = strrchr(entry->d_name, '.');
        if (!dot || strcmp(dot, ".fson") != 0)
            continue;

        char id[FOSSIL_BLUECRAB_MAX_ID];
        size_t len = dot - entry->d_name;
        if (len >= sizeof(id)) len = sizeof(id) - 1;
        strncpy(id, entry->d_name, len);
        id[len] = '\0';

        if (fossil_db_bluecrab_verify_entry(db, id) != 0)
            errors++;
    }
    closedir(dir);

    return errors == 0 ? 0 : -3;
}
