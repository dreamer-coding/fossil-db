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
#include "fossil/db/bluecrab.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

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
    fread(buffer, 1, size, f);

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

    mkdir(path);

    snprintf(buffer, sizeof(buffer), "%s/objects", path);
    mkdir(buffer);

    snprintf(buffer, sizeof(buffer), "%s/commits", path);
    mkdir(buffer);

    snprintf(buffer, sizeof(buffer), "%s/meta.fson", path);

    char meta[512];

    snprintf(meta, sizeof(meta),
    "{\n"
    "  database: cstr: \"%s\",\n"
    "  created: datetime: \"%ld\",\n"
    "  entries: array: []\n"
    "}\n",
    name,
    time(NULL));

    return bluecrab_write_file(buffer, meta);
}


int fossil_db_bluecrab_open(
    fossil_bluecrab_db *db,
    const char *path
)
{
    if (!db || !path) return -1;

    memset(db, 0, sizeof(*db));

    strncpy(db->root_path, path, FOSSIL_BLUECRAB_PATH - 1);

    snprintf(
        db->meta_path,
        sizeof(db->meta_path),
        "%s/meta.fson",
        path
    );

    if (!bluecrab_file_exists(db->meta_path))
        return -1;

    db->opened = true;

    return 0;
}


int fossil_db_bluecrab_close(
    fossil_bluecrab_db *db
)
{
    if (!db) return -1;

    db->opened = false;

    return 0;
}


int fossil_db_bluecrab_delete(
    const char *path
)
{
    /* placeholder */
    (void)path;
    return 0;
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
        return -2;

    return bluecrab_write_file(path, fson_data);
}


int fossil_db_bluecrab_get(
    fossil_bluecrab_db *db,
    const char *id,
    char **out_fson
)
{
    if (!db || !out_fson) return -1;

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
    if (!db) return -1;

    char path[FOSSIL_BLUECRAB_PATH];

    bluecrab_entry_path(db, id, path);

    return bluecrab_write_file(path, fson_data);
}


int fossil_db_bluecrab_remove(
    fossil_bluecrab_db *db,
    const char *id
)
{
    if (!db) return -1;

    char path[FOSSIL_BLUECRAB_PATH];

    bluecrab_entry_path(db, id, path);

    return remove(path);
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
    char id[FOSSIL_BLUECRAB_MAX_ID];

    snprintf(id, sizeof(id), "%s_%s", parent_id, sub_id);

    return fossil_db_bluecrab_insert(db, id, fson_data);
}


int fossil_db_bluecrab_get_sub(
    fossil_bluecrab_db *db,
    const char *parent_id,
    const char *sub_id,
    char **out_fson
)
{
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
    char path[FOSSIL_BLUECRAB_PATH];

    snprintf(path,
             sizeof(path),
             "%s/relations.fson",
             db->root_path);

    FILE *f = fopen(path, "a");

    if (!f) return -1;

    fprintf(f,
        "{ source: cstr:\"%s\", target: cstr:\"%s\", type:cstr:\"%s\" }\n",
        source_id,
        target_id,
        relation);

    fclose(f);

    return 0;
}


int fossil_db_bluecrab_unlink(
    fossil_bluecrab_db *db,
    const char *source_id,
    const char *target_id
)
{
    /* future implementation */
    (void)db;
    (void)source_id;
    (void)target_id;

    return 0;
}


int fossil_db_bluecrab_get_relations(
    fossil_bluecrab_db *db,
    const char *id,
    fossil_bluecrab_relation **out_relations,
    size_t *count
)
{
    /* placeholder */
    (void)db;
    (void)id;
    (void)out_relations;
    (void)count;

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
    /* placeholder */
    (void)db;
    (void)field;
    (void)value;
    (void)results;
    (void)count;

    return 0;
}


int fossil_db_bluecrab_search_fuzzy(
    fossil_bluecrab_db *db,
    const char *query,
    fossil_bluecrab_search_result **results,
    size_t *count
)
{
    /* placeholder */
    (void)db;
    (void)query;
    (void)results;
    (void)count;

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
    size_t la = strlen(a);
    size_t lb = strlen(b);

    if (la == 0 || lb == 0)
        return 0.0f;

    size_t match = 0;

    for (size_t i = 0; i < la && i < lb; i++)
    {
        if (a[i] == b[i])
            match++;
    }

    return (float)match / (float)(la > lb ? la : lb);
}


int fossil_db_bluecrab_rank_results(
    fossil_bluecrab_search_result *results,
    size_t count
)
{
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
    char *data = NULL;

    if (fossil_db_bluecrab_get(db, id, &data))
        return -1;

    char hash[FOSSIL_BLUECRAB_HASH_SIZE];

    fossil_db_bluecrab_hash_entry(data, hash);

    free(data);

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
    char path[FOSSIL_BLUECRAB_PATH];

    snprintf(path,
             sizeof(path),
             "%s/commits/%ld.fson",
             db->root_path,
             time(NULL));

    char commit[512];

    snprintf(commit,
    sizeof(commit),
    "{ message:cstr:\"%s\", time:u64:%ld }\n",
    message,
    time(NULL));

    return bluecrab_write_file(path, commit);
}


int fossil_db_bluecrab_log(
    fossil_bluecrab_db *db
)
{
    /* placeholder */
    (void)db;
    return 0;
}


int fossil_db_bluecrab_checkout(
    fossil_bluecrab_db *db,
    const char *version
)
{
    /* placeholder */
    (void)db;
    (void)version;

    return 0;
}


/* -----------------------------------------------------------
Meta
----------------------------------------------------------- */

int fossil_db_bluecrab_meta_load(
    fossil_bluecrab_db *db
)
{
    char *meta = bluecrab_read_file(db->meta_path);

    if (!meta)
        return -1;

    free(meta);

    return 0;
}


int fossil_db_bluecrab_meta_save(
    fossil_bluecrab_db *db
)
{
    /* placeholder */
    (void)db;
    return 0;
}


int fossil_db_bluecrab_meta_rebuild(
    fossil_bluecrab_db *db
)
{
    /* placeholder */
    (void)db;
    return 0;
}


/* -----------------------------------------------------------
Maintenance
----------------------------------------------------------- */

int fossil_db_bluecrab_backup(
    fossil_bluecrab_db *db,
    const char *backup_path
)
{
    (void)db;
    (void)backup_path;

    return 0;
}


int fossil_db_bluecrab_restore(
    fossil_bluecrab_db *db,
    const char *backup_path
)
{
    (void)db;
    (void)backup_path;

    return 0;
}


int fossil_db_bluecrab_compact(
    fossil_bluecrab_db *db
)
{
    (void)db;

    return 0;
}


int fossil_db_bluecrab_verify(
    fossil_bluecrab_db *db
)
{
    (void)db;

    return 0;
}
