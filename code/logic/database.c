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

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

char *_strdup_(const char *s)
{
    if (!s)
        return NULL;
    size_t len = strlen(s);
    char *dup = malloc(len + 1);
    if (dup)
        memcpy(dup, s, len + 1);
    return dup;
}

/*
------------------------------------------------------------
Internal Structures
------------------------------------------------------------
*/

typedef struct entry_node
{
    char *id;
    char *data;
    struct entry_node *next;
} entry_node;

typedef struct relation_node
{
    char *src;
    char *dst;
    char *rel;
    struct relation_node *next;
} relation_node;

typedef struct internal_state
{
    entry_node *entries;
    relation_node *relations;
} internal_state;

/*
------------------------------------------------------------
Helpers
------------------------------------------------------------
*/

static void set_error(fossil_db_t *db, const char *msg)
{
    if (!db)
        return;
    snprintf(db->last_error, FOSSIL_DB_MAX_ERROR, "%s", msg);
}

static entry_node *find_entry(internal_state *st, const char *id)
{
    entry_node *cur = st->entries;
    while (cur)
    {
        if (strcmp(cur->id, id) == 0)
            return cur;
        cur = cur->next;
    }
    return NULL;
}

/*
------------------------------------------------------------
Lifecycle
------------------------------------------------------------
*/

int fossil_db_database_create(const char *path, const char *name)
{
    if (!path || !name)
        return -1;

    /* simulate creation by touching a file */
    char full[FOSSIL_DB_MAX_PATH];
    snprintf(full, sizeof(full), "%s/%s.crab", path, name);

    FILE *f = fopen(full, "w");
    if (!f)
        return -1;
    fclose(f);

    return 0;
}

int fossil_db_database_open(fossil_db_t *db, const char *path)
{
    if (!db || !path)
        return -1;

    memset(db, 0, sizeof(*db));

    strncpy(db->path, path, FOSSIL_DB_MAX_PATH - 1);
    strncpy(db->name, path, FOSSIL_DB_MAX_NAME - 1);

    internal_state *st = calloc(1, sizeof(internal_state));
    if (!st)
        return -1;

    db->internal = st;
    db->opened = true;
    db->version = 1;

    return 0;
}

int fossil_db_database_close(fossil_db_t *db)
{
    if (!db || !db->opened)
        return -1;

    internal_state *st = db->internal;

    entry_node *e = st->entries;
    while (e)
    {
        entry_node *n = e->next;
        free(e->id);
        free(e->data);
        free(e);
        e = n;
    }

    relation_node *r = st->relations;
    while (r)
    {
        relation_node *n = r->next;
        free(r->src);
        free(r->dst);
        free(r->rel);
        free(r);
        r = n;
    }

    free(st);

    db->internal = NULL;
    db->opened = false;

    return 0;
}

int fossil_db_database_delete(const char *path)
{
    if (!path)
        return -1;
    return remove(path);
}

/*
------------------------------------------------------------
Entry Operations
------------------------------------------------------------
*/

int fossil_db_database_insert(fossil_db_t *db, const char *id, const char *data)
{
    if (!db || !id || !data)
        return -1;

    internal_state *st = db->internal;

    if (find_entry(st, id))
    {
        set_error(db, "entry already exists");
        return -1;
    }

    entry_node *e = calloc(1, sizeof(*e));
    e->id = _strdup_(id);
    e->data = _strdup_(data);

    e->next = st->entries;
    st->entries = e;

    db->entry_count++;
    return 0;
}

int fossil_db_database_get(fossil_db_t *db, const char *id, char **out_data)
{
    if (!db || !id || !out_data)
        return -1;

    internal_state *st = db->internal;
    entry_node *e = find_entry(st, id);

    if (!e)
    {
        set_error(db, "entry not found");
        return -1;
    }

    *out_data = _strdup_(e->data);
    return 0;
}

int fossil_db_database_update(fossil_db_t *db, const char *id, const char *data)
{
    if (!db || !id || !data)
        return -1;

    internal_state *st = db->internal;
    entry_node *e = find_entry(st, id);

    if (!e)
    {
        set_error(db, "entry not found");
        return -1;
    }

    free(e->data);
    e->data = _strdup_(data);

    return 0;
}

int fossil_db_database_remove(fossil_db_t *db, const char *id)
{
    if (!db || !id)
        return -1;

    internal_state *st = db->internal;
    entry_node **cur = &st->entries;

    while (*cur)
    {
        if (strcmp((*cur)->id, id) == 0)
        {
            entry_node *tmp = *cur;
            *cur = tmp->next;

            free(tmp->id);
            free(tmp->data);
            free(tmp);

            db->entry_count--;
            return 0;
        }
        cur = &(*cur)->next;
    }

    set_error(db, "entry not found");
    return -1;
}

/*
------------------------------------------------------------
Sub-Entry (simple namespacing)
------------------------------------------------------------
*/

int fossil_db_database_insert_sub(
    fossil_db_t *db,
    const char *parent_id,
    const char *sub_id,
    const char *data)
{
    if (!parent_id || !sub_id)
        return -1;

    char composed[256];
    snprintf(composed, sizeof(composed), "%s:%s", parent_id, sub_id);

    return fossil_db_database_insert(db, composed, data);
}

int fossil_db_database_get_sub(
    fossil_db_t *db,
    const char *parent_id,
    const char *sub_id,
    char **out_data)
{
    if (!parent_id || !sub_id)
        return -1;

    char composed[256];
    snprintf(composed, sizeof(composed), "%s:%s", parent_id, sub_id);

    return fossil_db_database_get(db, composed, out_data);
}

/*
------------------------------------------------------------
Relationships
------------------------------------------------------------
*/

int fossil_db_database_link(
    fossil_db_t *db,
    const char *source_id,
    const char *target_id,
    const char *relation)
{
    if (!db || !source_id || !target_id || !relation)
        return -1;

    internal_state *st = db->internal;

    relation_node *r = calloc(1, sizeof(*r));
    r->src = _strdup_(source_id);
    r->dst = _strdup_(target_id);
    r->rel = _strdup_(relation);

    r->next = st->relations;
    st->relations = r;

    db->relation_count++;
    return 0;
}

int fossil_db_database_unlink(
    fossil_db_t *db,
    const char *source_id,
    const char *target_id)
{
    if (!db || !source_id || !target_id)
        return -1;

    internal_state *st = db->internal;
    relation_node **cur = &st->relations;

    while (*cur)
    {
        if (strcmp((*cur)->src, source_id) == 0 &&
            strcmp((*cur)->dst, target_id) == 0)
        {

            relation_node *tmp = *cur;
            *cur = tmp->next;

            free(tmp->src);
            free(tmp->dst);
            free(tmp->rel);
            free(tmp);

            db->relation_count--;
            return 0;
        }
        cur = &(*cur)->next;
    }

    return -1;
}

/*
------------------------------------------------------------
Query / DSL (basic passthrough)
------------------------------------------------------------
*/

int fossil_db_database_query(
    fossil_db_t *db,
    const char *query,
    char **out_result)
{
    if (!db || !query || !out_result)
        return -1;

    /* simple echo for now */
    *out_result = _strdup_(query);
    return 0;
}

/*
------------------------------------------------------------
Search (naive contains)
------------------------------------------------------------
*/

int fossil_db_database_search(
    fossil_db_t *db,
    const char *field,
    const char *value,
    fossil_db_database_search_result **results,
    size_t *count)
{
    if (!db || !field || !value || !results || !count)
        return -1;

    internal_state *st = db->internal;

    size_t cap = 8;
    *results = malloc(sizeof(**results) * cap);
    *count = 0;

    entry_node *e = st->entries;
    while (e)
    {
        if (strstr(e->data, value))
        {
            if (*count >= cap)
            {
                cap *= 2;
                *results = realloc(*results, sizeof(**results) * cap);
            }

            strncpy((*results)[*count].id, e->id, FOSSIL_DB_MAX_NAME - 1);
            (*results)[*count].score = 1.0f;
            strncpy((*results)[*count].snippet, e->data, 127);

            (*count)++;
        }
        e = e->next;
    }

    return 0;
}

/*
------------------------------------------------------------
Versioning
------------------------------------------------------------
*/

int fossil_db_database_commit(fossil_db_t *db, const char *message)
{
    if (!db || !message)
        return -1;
    db->version++;
    return 0;
}

int fossil_db_database_checkout(fossil_db_t *db, const char *version)
{
    if (!db || !version)
        return -1;
    return 0;
}

int fossil_db_database_log(fossil_db_t *db)
{
    if (!db)
        return -1;
    printf("version: %llu\n", (unsigned long long)db->version);
    return 0;
}

/*
------------------------------------------------------------
Integrity
------------------------------------------------------------
*/

int fossil_db_database_verify(fossil_db_t *db)
{
    if (!db)
        return -1;
    return db->opened ? 0 : -1;
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
    return 0;
}

int fossil_db_database_backup(
    fossil_db_t *db,
    const char *backup_path)
{
    if (!db || !backup_path)
        return -1;

    FILE *f = fopen(backup_path, "w");
    if (!f)
        return -1;

    fprintf(f, "backup version %llu\n",
            (unsigned long long)db->version);

    fclose(f);
    return 0;
}

int fossil_db_database_restore(
    fossil_db_t *db,
    const char *backup_path)
{
    if (!db || !backup_path)
        return -1;
    return 0;
}

/*
------------------------------------------------------------
Media
------------------------------------------------------------
*/

int fossil_db_database_export(
    fossil_db_t *db,
    const char *format,
    const char *output_path)
{
    if (!db || !format || !output_path)
        return -1;

    FILE *f = fopen(output_path, "w");
    if (!f)
        return -1;

    fprintf(f, "export format: %s\n", format);
    fclose(f);

    return 0;
}

int fossil_db_database_import(
    fossil_db_t *db,
    const char *format,
    const char *input_path)
{
    if (!db || !format || !input_path)
        return -1;
    return 0;
}

/*
------------------------------------------------------------
Error
------------------------------------------------------------
*/

const char *fossil_db_database_last_error(fossil_db_t *db)
{
    if (!db)
        return "no db";
    return db->last_error;
}
