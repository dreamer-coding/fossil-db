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
 * Date: 04/05/2013
 *
 * Copyright (C) 2013-Current Fossil Logic. All rights reserved.
 * -----------------------------------------------------------------------------
 */
#include "fossil/crabdb/crabdb.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ============================================================
 * Internal Constants
 * ============================================================ */

#define FOSSIL_DB_CRABDB_INITIAL_TABLE_CAPACITY 8
#define FOSSIL_DB_CRABDB_MAX_ERROR_LENGTH 256

/* ============================================================
 * Internal Structures
 * ============================================================ */

struct fossil_db_crabdb_table_s
{
    char *name;
};

struct fossil_db_crabdb_record_s
{
    void *data;
};

struct fossil_db_crabdb_field_s
{
    char *name;
};

struct fossil_db_crabdb_value_s
{
    fossil_db_crabdb_type_t type;
    void *data;
};

struct fossil_db_crabdb_query_s
{
    char *query;
};

struct fossil_db_crabdb_result_s
{
    size_t count;
};

struct fossil_db_crabdb_transaction_s
{
    bool active;
};

struct fossil_db_crabdb_s
{
    char *path;

    bool memory;
    bool closed;
    bool transaction_active;

    fossil_db_crabdb_table_t **tables;
    size_t table_count;
    size_t table_capacity;

    char error[FOSSIL_DB_CRABDB_MAX_ERROR_LENGTH];
};

/* ============================================================
 * Internal Helpers
 * ============================================================ */

static char *
crabdb_strdup(const char *string)
{
    size_t length;
    char *copy;

    if (string == NULL)
    {
        return NULL;
    }

    length = strlen(string);

    copy = malloc(length + 1);

    if (copy == NULL)
    {
        return NULL;
    }

    memcpy(copy, string, length + 1);

    return copy;
}

static void
crabdb_set_error(
    fossil_db_crabdb_t *db,
    const char *message)
{
    if (db == NULL)
    {
        return;
    }

    if (message == NULL)
    {
        db->error[0] = '\0';
        return;
    }

    snprintf(
        db->error,
        sizeof(db->error),
        "%s",
        message);
}

static bool
crabdb_valid_db(
    fossil_db_crabdb_t *db)
{
    if (db == NULL)
    {
        return false;
    }

    if (db->closed)
    {
        return false;
    }

    return true;
}

static fossil_db_crabdb_table_t *
crabdb_find_table(
    fossil_db_crabdb_t *db,
    const char *name)
{
    size_t i;

    if (db == NULL || name == NULL)
    {
        return NULL;
    }

    for (i = 0; i < db->table_count; ++i)
    {
        if (db->tables[i] == NULL)
        {
            continue;
        }

        if (strcmp(db->tables[i]->name, name) == 0)
        {
            return db->tables[i];
        }
    }

    return NULL;
}

static fossil_db_crabdb_status_t
crabdb_grow_tables(
    fossil_db_crabdb_t *db)
{
    size_t capacity;
    fossil_db_crabdb_table_t **tables;

    if (db == NULL)
    {
        return FOSSIL_DB_CRABDB_INVALID_ARGUMENT;
    }

    capacity = db->table_capacity == 0
                   ? FOSSIL_DB_CRABDB_INITIAL_TABLE_CAPACITY
                   : db->table_capacity * 2;

    tables = realloc(
        db->tables,
        sizeof(*tables) * capacity);

    if (tables == NULL)
    {
        crabdb_set_error(
            db,
            "Unable to allocate table registry.");

        return FOSSIL_DB_CRABDB_OUT_OF_MEMORY;
    }

    db->tables = tables;
    db->table_capacity = capacity;

    return FOSSIL_DB_CRABDB_SUCCESS;
}

static fossil_db_crabdb_status_t
crabdb_allocate(
    fossil_db_crabdb_t **db,
    const char *path,
    bool memory)
{
    fossil_db_crabdb_t *instance;

    if (db == NULL)
    {
        return FOSSIL_DB_CRABDB_INVALID_ARGUMENT;
    }

    *db = NULL;

    instance = calloc(
        1,
        sizeof(*instance));

    if (instance == NULL)
    {
        return FOSSIL_DB_CRABDB_OUT_OF_MEMORY;
    }

    instance->memory = memory;
    instance->closed = false;
    instance->transaction_active = false;

    if (path != NULL)
    {
        instance->path = crabdb_strdup(path);

        if (instance->path == NULL)
        {
            free(instance);
            return FOSSIL_DB_CRABDB_OUT_OF_MEMORY;
        }
    }

    instance->table_capacity =
        FOSSIL_DB_CRABDB_INITIAL_TABLE_CAPACITY;

    instance->tables = calloc(
        instance->table_capacity,
        sizeof(*instance->tables));

    if (instance->tables == NULL)
    {
        free(instance->path);
        free(instance);

        return FOSSIL_DB_CRABDB_OUT_OF_MEMORY;
    }

    instance->error[0] = '\0';

    *db = instance;

    return FOSSIL_DB_CRABDB_SUCCESS;
}

/* ============================================================
 * Version
 * ============================================================ */

const char *
fossil_db_crabdb_version(void)
{
    return FOSSIL_DB_CRABDB_VERSION;
}

/* ============================================================
 * Status
 * ============================================================ */

const char *
fossil_db_crabdb_status_string(
    fossil_db_crabdb_status_t status)
{
    switch (status)
    {
    case FOSSIL_DB_CRABDB_SUCCESS:
        return "success";

    case FOSSIL_DB_CRABDB_ERROR:
        return "error";

    case FOSSIL_DB_CRABDB_INVALID_ARGUMENT:
        return "invalid argument";

    case FOSSIL_DB_CRABDB_OUT_OF_MEMORY:
        return "out of memory";

    case FOSSIL_DB_CRABDB_NOT_FOUND:
        return "not found";

    case FOSSIL_DB_CRABDB_ALREADY_EXISTS:
        return "already exists";

    case FOSSIL_DB_CRABDB_EXISTS:
        return "exists";

    case FOSSIL_DB_CRABDB_INVALID_STATE:
        return "invalid state";

    case FOSSIL_DB_CRABDB_IO_ERROR:
        return "I/O error";

    case FOSSIL_DB_CRABDB_CORRUPTED:
        return "database corrupted";

    case FOSSIL_DB_CRABDB_READ_ONLY:
        return "read only";

    case FOSSIL_DB_CRABDB_TRANSACTION_ERROR:
        return "transaction error";

    case FOSSIL_DB_CRABDB_QUERY_ERROR:
        return "query error";

    default:
        return "unknown status";
    }
}

/* ============================================================
 * Database
 * ============================================================ */

fossil_db_crabdb_status_t
fossil_db_crabdb_create(
    fossil_db_crabdb_t **db,
    const char *path)
{
    fossil_db_crabdb_status_t status;
    FILE *file;

    if (db == NULL || path == NULL || path[0] == '\0')
    {
        return FOSSIL_DB_CRABDB_INVALID_ARGUMENT;
    }

    file = fopen(path, "wb");

    if (file == NULL)
    {
        return FOSSIL_DB_CRABDB_IO_ERROR;
    }

    fclose(file);

    status = crabdb_allocate(
        db,
        path,
        false);

    return status;
}

fossil_db_crabdb_status_t
fossil_db_crabdb_open(
    fossil_db_crabdb_t **db,
    const char *path)
{
    FILE *file;

    if (db == NULL || path == NULL || path[0] == '\0')
    {
        return FOSSIL_DB_CRABDB_INVALID_ARGUMENT;
    }

    file = fopen(path, "rb");

    if (file == NULL)
    {
        return FOSSIL_DB_CRABDB_NOT_FOUND;
    }

    fclose(file);

    return crabdb_allocate(
        db,
        path,
        false);
}

fossil_db_crabdb_status_t
fossil_db_crabdb_open_memory(
    fossil_db_crabdb_t **db)
{
    return crabdb_allocate(
        db,
        NULL,
        true);
}

fossil_db_crabdb_status_t
fossil_db_crabdb_close(
    fossil_db_crabdb_t *db)
{
    size_t i;

    if (db == NULL)
    {
        return FOSSIL_DB_CRABDB_INVALID_ARGUMENT;
    }

    if (db->closed)
    {
        return FOSSIL_DB_CRABDB_INVALID_STATE;
    }

    for (i = 0; i < db->table_count; ++i)
    {
        if (db->tables[i] == NULL)
        {
            continue;
        }

        free(db->tables[i]->name);
        free(db->tables[i]);
    }

    free(db->tables);

    db->tables = NULL;
    db->table_count = 0;
    db->table_capacity = 0;

    db->closed = true;

    return FOSSIL_DB_CRABDB_SUCCESS;
}

void fossil_db_crabdb_destroy(
    fossil_db_crabdb_t *db)
{
    if (db == NULL)
    {
        return;
    }

    if (!db->closed)
    {
        fossil_db_crabdb_close(db);
    }

    free(db->path);
    free(db);
}

/* ============================================================
 * Database Information
 * ============================================================ */

fossil_db_crabdb_status_t
fossil_db_crabdb_last_error(
    fossil_db_crabdb_t *db,
    const char **message)
{
    if (db == NULL || message == NULL)
    {
        return FOSSIL_DB_CRABDB_INVALID_ARGUMENT;
    }

    *message = db->error;

    return FOSSIL_DB_CRABDB_SUCCESS;
}

/* ============================================================
 * Tables
 * ============================================================ */

fossil_db_crabdb_status_t
fossil_db_crabdb_create_table(
    fossil_db_crabdb_t *db,
    const char *name)
{
    fossil_db_crabdb_table_t *table;
    fossil_db_crabdb_status_t status;

    if (!crabdb_valid_db(db) || name == NULL || name[0] == '\0')
    {
        return FOSSIL_DB_CRABDB_INVALID_ARGUMENT;
    }

    if (crabdb_find_table(db, name) != NULL)
    {
        crabdb_set_error(
            db,
            "Table already exists.");

        return FOSSIL_DB_CRABDB_ALREADY_EXISTS;
    }

    if (db->table_count >= db->table_capacity)
    {
        status = crabdb_grow_tables(db);

        if (status != FOSSIL_DB_CRABDB_SUCCESS)
        {
            return status;
        }
    }

    table = calloc(
        1,
        sizeof(*table));

    if (table == NULL)
    {
        crabdb_set_error(
            db,
            "Unable to allocate table.");

        return FOSSIL_DB_CRABDB_OUT_OF_MEMORY;
    }

    table->name = crabdb_strdup(name);

    if (table->name == NULL)
    {
        free(table);

        crabdb_set_error(
            db,
            "Unable to allocate table name.");

        return FOSSIL_DB_CRABDB_OUT_OF_MEMORY;
    }

    db->tables[db->table_count++] = table;

    return FOSSIL_DB_CRABDB_SUCCESS;
}

fossil_db_crabdb_status_t
fossil_db_crabdb_drop_table(
    fossil_db_crabdb_t *db,
    const char *name)
{
    size_t i;
    fossil_db_crabdb_table_t *table;

    if (!crabdb_valid_db(db) || name == NULL)
    {
        return FOSSIL_DB_CRABDB_INVALID_ARGUMENT;
    }

    for (i = 0; i < db->table_count; ++i)
    {
        table = db->tables[i];

        if (table == NULL)
        {
            continue;
        }

        if (strcmp(table->name, name) != 0)
        {
            continue;
        }

        free(table->name);
        free(table);

        for (; i + 1 < db->table_count; ++i)
        {
            db->tables[i] = db->tables[i + 1];
        }

        db->tables[db->table_count - 1] = NULL;
        db->table_count--;

        return FOSSIL_DB_CRABDB_SUCCESS;
    }

    crabdb_set_error(
        db,
        "Table was not found.");

    return FOSSIL_DB_CRABDB_NOT_FOUND;
}

fossil_db_crabdb_status_t
fossil_db_crabdb_rename_table(
    fossil_db_crabdb_t *db,
    const char *old_name,
    const char *new_name)
{
    fossil_db_crabdb_table_t *table;
    char *name;

    if (!crabdb_valid_db(db) ||
        old_name == NULL ||
        new_name == NULL ||
        old_name[0] == '\0' ||
        new_name[0] == '\0')
    {
        return FOSSIL_DB_CRABDB_INVALID_ARGUMENT;
    }

    table = crabdb_find_table(
        db,
        old_name);

    if (table == NULL)
    {
        crabdb_set_error(
            db,
            "Table was not found.");

        return FOSSIL_DB_CRABDB_NOT_FOUND;
    }

    if (crabdb_find_table(db, new_name) != NULL)
    {
        crabdb_set_error(
            db,
            "Destination table already exists.");

        return FOSSIL_DB_CRABDB_ALREADY_EXISTS;
    }

    name = crabdb_strdup(new_name);

    if (name == NULL)
    {
        return FOSSIL_DB_CRABDB_OUT_OF_MEMORY;
    }

    free(table->name);
    table->name = name;

    return FOSSIL_DB_CRABDB_SUCCESS;
}

bool fossil_db_crabdb_table_exists(
    fossil_db_crabdb_t *db,
    const char *name)
{
    if (!crabdb_valid_db(db) || name == NULL)
    {
        return false;
    }

    return crabdb_find_table(
               db,
               name) != NULL;
}

/* ============================================================
 * Records
 * ============================================================ */

fossil_db_crabdb_status_t
fossil_db_crabdb_insert(
    fossil_db_crabdb_t *db,
    const char *table,
    fossil_db_crabdb_record_t *record)
{
    if (!crabdb_valid_db(db) ||
        table == NULL ||
        record == NULL)
    {
        return FOSSIL_DB_CRABDB_INVALID_ARGUMENT;
    }

    if (!fossil_db_crabdb_table_exists(db, table))
    {
        crabdb_set_error(
            db,
            "Insert target table was not found.");

        return FOSSIL_DB_CRABDB_NOT_FOUND;
    }

    /*
     * Record storage will be implemented once the public
     * record/field API is established.
     */
    crabdb_set_error(
        db,
        "Record storage is not yet implemented.");

    return FOSSIL_DB_CRABDB_ERROR;
}

fossil_db_crabdb_status_t
fossil_db_crabdb_update(
    fossil_db_crabdb_t *db,
    const char *table,
    fossil_db_crabdb_record_t *record)
{
    if (!crabdb_valid_db(db) ||
        table == NULL ||
        record == NULL)
    {
        return FOSSIL_DB_CRABDB_INVALID_ARGUMENT;
    }

    if (!fossil_db_crabdb_table_exists(db, table))
    {
        return FOSSIL_DB_CRABDB_NOT_FOUND;
    }

    crabdb_set_error(
        db,
        "Record storage is not yet implemented.");

    return FOSSIL_DB_CRABDB_ERROR;
}

fossil_db_crabdb_status_t
fossil_db_crabdb_delete(
    fossil_db_crabdb_t *db,
    const char *table,
    fossil_db_crabdb_record_t *record)
{
    if (!crabdb_valid_db(db) ||
        table == NULL ||
        record == NULL)
    {
        return FOSSIL_DB_CRABDB_INVALID_ARGUMENT;
    }

    if (!fossil_db_crabdb_table_exists(db, table))
    {
        return FOSSIL_DB_CRABDB_NOT_FOUND;
    }

    crabdb_set_error(
        db,
        "Record storage is not yet implemented.");

    return FOSSIL_DB_CRABDB_ERROR;
}

fossil_db_crabdb_status_t
fossil_db_crabdb_select(
    fossil_db_crabdb_t *db,
    const char *table,
    fossil_db_crabdb_result_t **result)
{
    if (!crabdb_valid_db(db) ||
        table == NULL ||
        result == NULL)
    {
        return FOSSIL_DB_CRABDB_INVALID_ARGUMENT;
    }

    *result = NULL;

    if (!fossil_db_crabdb_table_exists(db, table))
    {
        return FOSSIL_DB_CRABDB_NOT_FOUND;
    }

    crabdb_set_error(
        db,
        "Record storage is not yet implemented.");

    return FOSSIL_DB_CRABDB_ERROR;
}

/* ============================================================
 * Values
 * ============================================================ */

fossil_db_crabdb_status_t
fossil_db_crabdb_value_create(
    fossil_db_crabdb_value_t **value,
    fossil_db_crabdb_type_t type)
{
    fossil_db_crabdb_value_t *instance;

    if (value == NULL)
    {
        return FOSSIL_DB_CRABDB_INVALID_ARGUMENT;
    }

    *value = NULL;

    instance = calloc(
        1,
        sizeof(*instance));

    if (instance == NULL)
    {
        return FOSSIL_DB_CRABDB_OUT_OF_MEMORY;
    }

    instance->type = type;

    *value = instance;

    return FOSSIL_DB_CRABDB_SUCCESS;
}

void fossil_db_crabdb_value_destroy(
    fossil_db_crabdb_value_t *value)
{
    if (value == NULL)
    {
        return;
    }

    free(value->data);
    free(value);
}

fossil_db_crabdb_type_t
fossil_db_crabdb_value_type(
    const fossil_db_crabdb_value_t *value)
{
    if (value == NULL)
    {
        return FOSSIL_DB_CRABDB_TYPE_NULL;
    }

    return value->type;
}

/* ============================================================
 * Transactions
 * ============================================================ */

fossil_db_crabdb_status_t
fossil_db_crabdb_begin(
    fossil_db_crabdb_t *db)
{
    if (!crabdb_valid_db(db))
    {
        return FOSSIL_DB_CRABDB_INVALID_ARGUMENT;
    }

    if (db->transaction_active)
    {
        crabdb_set_error(
            db,
            "Transaction is already active.");

        return FOSSIL_DB_CRABDB_TRANSACTION_ERROR;
    }

    db->transaction_active = true;

    return FOSSIL_DB_CRABDB_SUCCESS;
}

fossil_db_crabdb_status_t
fossil_db_crabdb_commit(
    fossil_db_crabdb_t *db)
{
    if (!crabdb_valid_db(db))
    {
        return FOSSIL_DB_CRABDB_INVALID_ARGUMENT;
    }

    if (!db->transaction_active)
    {
        crabdb_set_error(
            db,
            "No active transaction.");

        return FOSSIL_DB_CRABDB_TRANSACTION_ERROR;
    }

    db->transaction_active = false;

    return FOSSIL_DB_CRABDB_SUCCESS;
}

fossil_db_crabdb_status_t
fossil_db_crabdb_rollback(
    fossil_db_crabdb_t *db)
{
    if (!crabdb_valid_db(db))
    {
        return FOSSIL_DB_CRABDB_INVALID_ARGUMENT;
    }

    if (!db->transaction_active)
    {
        crabdb_set_error(
            db,
            "No active transaction.");

        return FOSSIL_DB_CRABDB_TRANSACTION_ERROR;
    }

    db->transaction_active = false;

    return FOSSIL_DB_CRABDB_SUCCESS;
}

/* ============================================================
 * Query
 * ============================================================ */

fossil_db_crabdb_status_t
fossil_db_crabdb_query(
    fossil_db_crabdb_t *db,
    const char *query,
    fossil_db_crabdb_result_t **result)
{
    if (!crabdb_valid_db(db) ||
        query == NULL ||
        result == NULL)
    {
        return FOSSIL_DB_CRABDB_INVALID_ARGUMENT;
    }

    *result = NULL;

    if (query[0] == '\0')
    {
        crabdb_set_error(
            db,
            "Query is empty.");

        return FOSSIL_DB_CRABDB_QUERY_ERROR;
    }

    crabdb_set_error(
        db,
        "CrabQL query engine is not yet implemented.");

    return FOSSIL_DB_CRABDB_QUERY_ERROR;
}

fossil_db_crabdb_status_t
fossil_db_crabdb_execute(
    fossil_db_crabdb_t *db,
    const char *query)
{
    if (!crabdb_valid_db(db) || query == NULL)
    {
        return FOSSIL_DB_CRABDB_INVALID_ARGUMENT;
    }

    if (query[0] == '\0')
    {
        crabdb_set_error(
            db,
            "Query is empty.");

        return FOSSIL_DB_CRABDB_QUERY_ERROR;
    }

    crabdb_set_error(
        db,
        "CrabQL query engine is not yet implemented.");

    return FOSSIL_DB_CRABDB_QUERY_ERROR;
}

/* ============================================================
 * Results
 * ============================================================ */

size_t
fossil_db_crabdb_result_count(
    const fossil_db_crabdb_result_t *result)
{
    if (result == NULL)
    {
        return 0;
    }

    return result->count;
}

void fossil_db_crabdb_result_destroy(
    fossil_db_crabdb_result_t *result)
{
    if (result == NULL)
    {
        return;
    }

    free(result);
}
