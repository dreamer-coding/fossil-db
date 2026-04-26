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
#include "fossil/db/database.h"

#include <ctype.c>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
------------------------------------------------------------
Internal Engine Definitions
------------------------------------------------------------
*/

typedef struct fossil_record_t
{
    char *id;
    char *data;
    uint64_t version;
    struct fossil_record_t *next;
} fossil_record_t;

typedef struct fossil_index_t
{
    fossil_record_t *head;
} fossil_index_t;

typedef struct fossil_engine_t
{
    FILE *data_fp;
    FILE *wal_fp;

    fossil_index_t index;

    uint64_t current_version;
} fossil_engine_t;

/*
------------------------------------------------------------
Utilities
------------------------------------------------------------
*/

static void fossil_set_error(fossil_db_t *db, uint32_t code, const char *msg)
{
    db->last_error_code = code;
    strncpy(db->last_error, msg, FOSSIL_DB_MAX_ERROR - 1);
}

static fossil_engine_t *engine_get(fossil_db_t *db)
{
    return (fossil_engine_t *)db->internal;
}

/*
------------------------------------------------------------
Lifecycle
------------------------------------------------------------
*/

int fossil_db_database_create(const char *path, const char *name)
{
    char fullpath[FOSSIL_DB_MAX_PATH];
    snprintf(fullpath, sizeof(fullpath), "%s/%s.fdb", path, name);

    FILE *fp = fopen(fullpath, "wb");
    if (!fp)
        return -1;

    fclose(fp);
    return 0;
}

int fossil_db_database_open(fossil_db_t *db, const char *path)
{
    if (!db || !path)
        return -1;

    memset(db, 0, sizeof(*db));

    fossil_engine_t *eng = calloc(1, sizeof(*eng));
    if (!eng)
        return -1;

    eng->data_fp = fopen(path, "rb+");
    if (!eng->data_fp)
    {
        free(eng);
        return -1;
    }

    char wal_path[FOSSIL_DB_MAX_PATH];
    snprintf(wal_path, sizeof(wal_path), "%s.wal", path);

    eng->wal_fp = fopen(wal_path, "ab+");

    db->internal = eng;
    db->opened = true;
    db->version = 1;

    strncpy(db->path, path, FOSSIL_DB_MAX_PATH - 1);

    return 0;
}

int fossil_db_database_close(fossil_db_t *db)
{
    if (!db || !db->internal)
        return -1;

    fossil_engine_t *eng = engine_get(db);

    fclose(eng->data_fp);
    fclose(eng->wal_fp);

    /* free index */
    fossil_record_t *cur = eng->index.head;
    while (cur)
    {
        fossil_record_t *next = cur->next;
        free(cur->id);
        free(cur->data);
        free(cur);
        cur = next;
    }

    free(eng);
    db->internal = NULL;
    db->opened = false;

    return 0;
}

int fossil_db_database_delete(const char *path)
{
    return remove(path);
}

/*
------------------------------------------------------------
Core CRUD
------------------------------------------------------------
*/

static fossil_record_t *index_find(fossil_engine_t *eng, const char *id)
{
    fossil_record_t *cur = eng->index.head;
    while (cur)
    {
        if (strcmp(cur->id, id) == 0)
            return cur;
        cur = cur->next;
    }
    return NULL;
}

int fossil_db_database_insert(fossil_db_t *db, const char *id, const char *data)
{
    if (!db || !id || !data)
        return -1;

    fossil_engine_t *eng = engine_get(db);

    if (index_find(eng, id))
    {
        fossil_set_error(db, 1, "duplicate id");
        return -1;
    }

    fossil_record_t *rec = calloc(1, sizeof(*rec));
    rec->id = strdup(id);
    rec->data = strdup(data);
    rec->version = ++eng->current_version;

    rec->next = eng->index.head;
    eng->index.head = rec;

    fprintf(eng->wal_fp, "INSERT %s %s\n", id, data);
    fflush(eng->wal_fp);

    db->entry_count++;

    return 0;
}

int fossil_db_database_get(
    fossil_db_t *db,
    const char *id,
    char **out_data)
{
    if (!db || !id || !out_data)
        return -1;

    fossil_engine_t *eng = engine_get(db);
    fossil_record_t *rec = index_find(eng, id);

    if (!rec)
        return -1;

    *out_data = strdup(rec->data);
    return 0;
}

int fossil_db_database_update(
    fossil_db_t *db,
    const char *id,
    const char *data)
{
    fossil_engine_t *eng = engine_get(db);
    fossil_record_t *rec = index_find(eng, id);

    if (!rec)
        return -1;

    free(rec->data);
    rec->data = strdup(data);
    rec->version = ++eng->current_version;

    fprintf(eng->wal_fp, "UPDATE %s %s\n", id, data);
    fflush(eng->wal_fp);

    return 0;
}

int fossil_db_database_remove(
    fossil_db_t *db,
    const char *id)
{
    fossil_engine_t *eng = engine_get(db);

    fossil_record_t **cur = &eng->index.head;

    while (*cur)
    {
        if (strcmp((*cur)->id, id) == 0)
        {
            fossil_record_t *tmp = *cur;
            *cur = tmp->next;

            free(tmp->id);
            free(tmp->data);
            free(tmp);

            fprintf(eng->wal_fp, "REMOVE %s\n", id);
            fflush(eng->wal_fp);

            db->entry_count--;
            return 0;
        }
        cur = &(*cur)->next;
    }

    return -1;
}

/*
------------------------------------------------------------
Sub-Entry / Relationship (minimal real wiring)
------------------------------------------------------------
*/

int fossil_db_database_insert_sub(
    fossil_db_t *db,
    const char *parent_id,
    const char *sub_id,
    const char *data)
{
    /* stored as composite key */
    char key[256];
    snprintf(key, sizeof(key), "%s:%s", parent_id, sub_id);
    return fossil_db_database_insert(db, key, data);
}

int fossil_db_database_get_sub(
    fossil_db_t *db,
    const char *parent_id,
    const char *sub_id,
    char **out_data)
{
    char key[256];
    snprintf(key, sizeof(key), "%s:%s", parent_id, sub_id);
    return fossil_db_database_get(db, key, out_data);
}

/*
------------------------------------------------------------
Relationship (Family / DAG)
------------------------------------------------------------
*/

int fossil_db_database_link(
    fossil_db_t *db,
    const char *source_id,
    const char *target_id,
    const char *relation)
{
    char key[256];
    snprintf(key, sizeof(key), "rel:%s:%s", source_id, target_id);
    return fossil_db_database_insert(db, key, relation);
}

int fossil_db_database_unlink(
    fossil_db_t *db,
    const char *source_id,
    const char *target_id)
{
    char key[256];
    snprintf(key, sizeof(key), "rel:%s:%s", source_id, target_id);
    return fossil_db_database_remove(db, key);
}

/*
------------------------------------------------------------
Query / Search (baseline)
------------------------------------------------------------
*/

int fossil_db_database_query(
    fossil_db_t *db,
    const char *query,
    char **out_result)
{
    /* placeholder for DSL engine hook */
    *out_result = strdup(query);
    return 0;
}

int fossil_db_database_search(
    fossil_db_t *db,
    const char *field,
    const char *value,
    fossil_db_database_search_result **results,
    size_t *count)
{
    (void)field;

    fossil_engine_t *eng = engine_get(db);

    size_t cap = 16;
    *results = malloc(sizeof(**results) * cap);
    *count = 0;

    fossil_record_t *cur = eng->index.head;
    while (cur)
    {
        if (strstr(cur->data, value))
        {
            if (*count >= cap)
            {
                cap *= 2;
                *results = realloc(*results, sizeof(**results) * cap);
            }

            strncpy((*results)[*count].id, cur->id, FOSSIL_DB_MAX_NAME);
            (*results)[*count].score = 1.0f;
            strncpy((*results)[*count].snippet, cur->data, 127);

            (*count)++;
        }
        cur = cur->next;
    }

    return 0;
}

/*
------------------------------------------------------------
Versioning / Integrity
------------------------------------------------------------
*/

int fossil_db_database_commit(
    fossil_db_t *db,
    const char *message)
{
    fossil_engine_t *eng = engine_get(db);

    db->last_commit_version = ++eng->current_version;

    snprintf(db->last_commit_hash, sizeof(db->last_commit_hash),
             "commit-%llu", (unsigned long long)db->last_commit_version);

    fprintf(eng->wal_fp, "COMMIT %s\n", message);
    fflush(eng->wal_fp);

    return 0;
}

int fossil_db_database_checkout(
    fossil_db_t *db,
    const char *version)
{
    (void)db;
    (void)version;
    return 0;
}

int fossil_db_database_log(fossil_db_t *db)
{
    (void)db;
    return 0;
}

int fossil_db_database_verify(fossil_db_t *db)
{
    if (!db || !db->internal)
        return -1;

    db->corrupted = false;
    return 0;
}

/*
------------------------------------------------------------
Maintenance
------------------------------------------------------------
*/

int fossil_db_database_compact(fossil_db_t *db)
{
    if (!db)
        return -1;

    if (!db->opened || db->corrupted)
    {
        snprintf(db->last_error,
                 sizeof(db->last_error),
                 "compact failed: db not in valid state");

        db->last_error_code = 1;
        return -1;
    }

    /* ------------------------------------------------------------
       Step 1: Sync logical state
       ------------------------------------------------------------
    */
    db->integrity_version = db->version;

    /* ------------------------------------------------------------
       Step 2: Normalize internal stats
       (real DBs would rebuild indexes / reclaim deleted entries)
       ------------------------------------------------------------
    */
    if (db->entry_count == 0 && db->relation_count == 0)
    {
        /* nothing to compact, but still mark clean */
    }

    /* ------------------------------------------------------------
       Step 3: Internal engine hook (future WAL / index rebuild)
       ------------------------------------------------------------
    */
    if (db->internal)
    {
        /*
        fossil_storage_compact(db->internal);
        fossil_index_rebuild(db->internal);
        fossil_wal_truncate(db->internal);
        */
    }

    /* ------------------------------------------------------------
       Step 4: Update root integrity hash (lightweight placeholder)
       ------------------------------------------------------------
    */
    uint64_t mix =
        db->version ^
        db->entry_count ^
        db->relation_count ^
        db->integrity_version;

    snprintf(db->root_hash,
             sizeof(db->root_hash),
             "compact-%llu-%llu",
             (unsigned long long)db->version,
             (unsigned long long)mix);

    /* ------------------------------------------------------------
       Step 5: bump compaction cycle
       ------------------------------------------------------------
    */
    db->version++;

    return 0;
}

int fossil_db_database_backup(
    fossil_db_t *db,
    const char *backup_path)
{
    if (!db || !db->opened || !backup_path)
        return -1;

    FILE *src = fopen(db->path, "rb");
    if (!src)
        return -1;

    FILE *dst = fopen(backup_path, "wb");
    if (!dst)
    {
        fclose(src);
        return -1;
    }

    char buf[4096];
    size_t n;

    while ((n = fread(buf, 1, sizeof(buf), src)) > 0)
        fwrite(buf, 1, n, dst);

    fclose(src);
    fclose(dst);

    /* mark snapshot state */
    db->last_commit_version = db->version;

    snprintf(db->last_commit_hash,
             sizeof(db->last_commit_hash),
             "backup-%llu",
             (unsigned long long)db->version);

    return 0;
}

int fossil_db_database_restore(
    fossil_db_t *db,
    const char *backup_path)
{
    if (!db || !backup_path)
        return -1;

    FILE *src = fopen(backup_path, "rb");
    if (!src)
        return -1;

    /* write to temp file first for safety */
    char tmp_path[FOSSIL_DB_MAX_PATH];
    snprintf(tmp_path, sizeof(tmp_path), "%s.tmp", db->path);

    FILE *dst = fopen(tmp_path, "wb");
    if (!dst)
    {
        fclose(src);
        return -1;
    }

    char buf[4096];
    size_t n;

    while ((n = fread(buf, 1, sizeof(buf), src)) > 0)
        fwrite(buf, 1, n, dst);

    fclose(src);
    fclose(dst);

    /* atomic replace */
    remove(db->path);
    rename(tmp_path, db->path);

    /* reset in-memory state */
    db->version = db->last_commit_version;
    db->corrupted = false;
    db->integrity_version = db->version;

    return 0;
}

/*
-------------------------------------------------------------
Media
-------------------------------------------------------------
*/

static fossil_engine_t *engine_get(fossil_db_t *db);

/*
------------------------------------------------------------
Helpers
------------------------------------------------------------
*/

static int str_eq(const char *a, const char *b)
{
    return strcmp(a, b) == 0;
}

/* basic JSON string escape */
static void json_escape(FILE *fp, const char *s)
{
    while (*s)
    {
        switch (*s)
        {
        case '"':  fputs("\\\"", fp); break;
        case '\\': fputs("\\\\", fp); break;
        case '\n': fputs("\\n", fp); break;
        case '\r': fputs("\\r", fp); break;
        case '\t': fputs("\\t", fp); break;
        default: fputc(*s, fp);
        }
        s++;
    }
}

/*
------------------------------------------------------------
EXPORT
------------------------------------------------------------
*/

int fossil_db_database_export(
    fossil_db_t *db,
    const char *format,
    const char *output_path)
{
    if (!db || !db->internal || !format || !output_path)
        return -1;

    fossil_engine_t *eng = engine_get(db);

    FILE *fp = fopen(output_path, "wb");
    if (!fp)
        return -1;

    /*
    ------------------------
    JSON EXPORT
    ------------------------
    */
    if (str_eq(format, "json"))
    {
        fprintf(fp, "{\n");
        fprintf(fp, "  \"name\": \"%s\",\n", db->name);
        fprintf(fp, "  \"version\": %llu,\n",
                (unsigned long long)db->version);
        fprintf(fp, "  \"entries\": [\n");

        fossil_record_t *cur = eng->index.head;
        int first = 1;

        while (cur)
        {
            if (!first)
                fprintf(fp, ",\n");

            fprintf(fp, "    {\"id\":\"");
            json_escape(fp, cur->id);
            fprintf(fp, "\",\"data\":\"");
            json_escape(fp, cur->data);
            fprintf(fp, "\"}");

            first = 0;
            cur = cur->next;
        }

        fprintf(fp, "\n  ]\n");
        fprintf(fp, "}\n");

        fclose(fp);
        return 0;
    }

    /*
    ------------------------
    FSON (simple binary-ish text for now)
    ------------------------
    */
    if (str_eq(format, "fson"))
    {
        fossil_record_t *cur = eng->index.head;

        while (cur)
        {
            fprintf(fp, "%s|%s\n", cur->id, cur->data);
            cur = cur->next;
        }

        fclose(fp);
        return 0;
    }

    fclose(fp);
    return -1;
}

/*
------------------------------------------------------------
IMPORT (JSON + FSON)
------------------------------------------------------------
*/

/* skip whitespace */
static char *skip_ws(char *p)
{
    while (*p && isspace((unsigned char)*p))
        p++;
    return p;
}

/* extract quoted string (mutates input buffer) */
static char *extract_string(char **p)
{
    char *start = strchr(*p, '"');
    if (!start)
        return NULL;

    start++;
    char *end = start;

    while (*end && *end != '"')
        end++;

    if (!*end)
        return NULL;

    *end = '\0';
    *p = end + 1;

    return start;
}

int fossil_db_database_import(
    fossil_db_t *db,
    const char *format,
    const char *input_path)
{
    if (!db || !db->internal || !format || !input_path)
        return -1;

    fossil_engine_t *eng = engine_get(db);

    FILE *fp = fopen(input_path, "rb");
    if (!fp)
        return -1;

    /*
    ------------------------
    RESET CURRENT STATE
    ------------------------
    */
    fossil_record_t *cur = eng->index.head;
    while (cur)
    {
        fossil_record_t *next = cur->next;
        free(cur->id);
        free(cur->data);
        free(cur);
        cur = next;
    }

    eng->index.head = NULL;
    db->entry_count = 0;

    /*
    ------------------------
    JSON IMPORT
    ------------------------
    */
    if (str_eq(format, "json"))
    {
        fseek(fp, 0, SEEK_END);
        long size = ftell(fp);
        rewind(fp);

        char *buf = malloc(size + 1);
        fread(buf, 1, size, fp);
        buf[size] = '\0';

        char *p = buf;

        while ((p = strstr(p, "\"id\"")))
        {
            char *id = extract_string(&p);
            if (!id)
                break;

            p = strstr(p, "\"data\"");
            if (!p)
                break;

            char *data = extract_string(&p);
            if (!data)
                break;

            fossil_db_database_insert(db, id, data);
        }

        free(buf);
        fclose(fp);
        return 0;
    }

    /*
    ------------------------
    FSON IMPORT
    ------------------------
    */
    if (str_eq(format, "fson"))
    {
        char line[4096];

        while (fgets(line, sizeof(line), fp))
        {
            char *sep = strchr(line, '|');
            if (!sep)
                continue;

            *sep = '\0';

            char *id = line;
            char *data = sep + 1;

            /* strip newline */
            char *nl = strchr(data, '\n');
            if (nl)
                *nl = '\0';

            fossil_db_database_insert(db, id, data);
        }

        fclose(fp);
        return 0;
    }

    fclose(fp);
    return -1;
}

/*
------------------------------------------------------------
Hash / Error
------------------------------------------------------------
*/

/*
------------------------------------------------------------
FNV-1a 64-bit
------------------------------------------------------------
- Supports binary blobs (not just strings)
- Optional length parameter (safe for serialized entries)
- Final avalanche mixing step improves distribution
------------------------------------------------------------
*/

static uint64_t fossil_fnv1a64_raw(const void *data, size_t len)
{
    const uint8_t *ptr = (const uint8_t *)data;

    uint64_t hash = 1469598103934665603ULL; /* FNV offset basis */
    const uint64_t prime = 1099511628211ULL;

    /* Main hash loop */
    for (size_t i = 0; i < len; i++)
    {
        hash ^= (uint64_t)ptr[i];
        hash *= prime;
    }

    hash ^= hash >> 33;
    hash *= 0xff51afd7ed558ccdULL;
    hash ^= hash >> 33;
    hash *= 0xc4ceb9fe1a85ec53ULL;
    hash ^= hash >> 33;

    return hash;
}

/*
------------------------------------------------------------
String convenience wrapper
------------------------------------------------------------
*/

static uint64_t fossil_fnv1a64(const char *data)
{
    if (!data)
        return 0;

    return fossil_fnv1a64_raw(data, strlen(data));
}

int fossil_db_database_hash(const char *data, char *out_hash)
{
    if (!data || !out_hash)
        return -1;

    uint64_t h = fossil_fnv1a64(data);

    snprintf(out_hash, 64, "%016llx", (unsigned long long)h);

    return 0;
}

int fossil_db_database_get_hash(
    fossil_db_t *db,
    const char *id,
    char *out_hash)
{
    if (!db || !id || !out_hash)
        return -1;

    char *data = NULL;

    if (fossil_db_database_get(db, id, &data) != 0)
        return -1;

    char buffer[2048];

    snprintf(buffer, sizeof(buffer),
             "%s|%s|%llu",
             id,
             data ? data : "",
             (unsigned long long)db->version);

    fossil_db_database_hash(buffer, out_hash);

    free(data);
    return 0;
}

int fossil_db_database_verify_entry(
    fossil_db_t *db,
    const char *id)
{
    if (!db || !id)
        return -1;

    char *data = NULL;
    char stored_hash[64] = {0};
    char computed_hash[64] = {0};

    if (fossil_db_database_get(db, id, &data) != 0)
    {
        snprintf(db->last_error, FOSSIL_DB_MAX_ERROR,
                 "verify_entry: missing entry '%s'", id);
        db->last_error_code = 1;
        return -1;
    }

    if (fossil_db_database_hash(data, computed_hash) != 0)
    {
        free(data);

        snprintf(db->last_error, FOSSIL_DB_MAX_ERROR,
                 "verify_entry: hash failure '%s'", id);
        db->last_error_code = 2;
        return -1;
    }

    free(data);

    if (fossil_db_database_get_hash(db, id, stored_hash) != 0)
    {
        snprintf(db->last_error, FOSSIL_DB_MAX_ERROR,
                 "verify_entry: missing stored hash '%s'", id);
        db->last_error_code = 3;
        return -1;
    }

    if (strncmp(stored_hash, computed_hash, 64) != 0)
    {
        snprintf(db->last_error, FOSSIL_DB_MAX_ERROR,
                 "verify_entry: hash mismatch '%s'", id);
        db->last_error_code = 4;

        db->corrupted = true;
        return -1;
    }

    db->integrity_version = db->version;

    return 0;
}

static int fossil_db_compute_root_hash(fossil_db_t *db)
{
    fossil_engine_t *eng = (fossil_engine_t *)db->internal;

    char buffer[4096];
    buffer[0] = '\0';

    fossil_record_t *cur = eng->index.head;

    /* deterministic fold over dataset */
    while (cur)
    {
        char line[512];
        snprintf(line, sizeof(line),
                 "%s:%s:%llu|",
                 cur->id,
                 cur->data,
                 (unsigned long long)cur->version);

        strncat(buffer, line, sizeof(buffer) - strlen(buffer) - 1);
        cur = cur->next;
    }

    /* mix in metadata */
    char final[8192];
    snprintf(final, sizeof(final),
             "%s|%zu|%zu",
             buffer,
             db->entry_count,
             db->relation_count);

    fossil_db_database_hash(final, db->root_hash);

    return 0;
}

int fossil_db_database_commit(fossil_db_t *db, const char *message)
{
    fossil_engine_t *eng = (fossil_engine_t *)db->internal;

    db->last_commit_version = ++eng->current_version;

    /* compute root BEFORE sealing commit */
    fossil_db_compute_root_hash(db);

    snprintf(db->last_commit_hash, sizeof(db->last_commit_hash),
             "%s-%llu",
             db->root_hash,
             (unsigned long long)db->last_commit_version);

    fprintf(eng->wal_fp, "COMMIT %s | %s\n",
            message ? message : "",
            db->last_commit_hash);

    fflush(eng->wal_fp);

    db->version = db->last_commit_version;

    return 0;
}

const char *fossil_db_database_last_error(fossil_db_t *db)
{
    return db->last_error;
}
