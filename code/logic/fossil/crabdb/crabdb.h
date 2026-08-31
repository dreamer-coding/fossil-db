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
#ifndef FOSSIL_DB_CRABDB_H
#define FOSSIL_DB_CRABDB_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================
 * Blue Crab DB — Core Database Engine
 * ============================================================
 *
 * Blue Crab DB
 *
 * A cross-platform C database and persistent storage library
 * built on SQLite.
 *
 * Database files:
 *     *.crab
 *
 * Query language files:
 *     *.crabql
 *
 * CrabQL is intentionally outside of this core API.
 *
 * Design:
 *     - String-based type identifiers
 *     - Runtime values
 *     - Runtime configuration
 *     - No public type/operator enums
 *     - C-first API
 *     - C++ compatible
 *     - SQLite-backed persistence
 *
 * ============================================================ */


/* ============================================================
 * Opaque Handles
 * ============================================================ */

typedef struct fossil_db_crabdb_s        fossil_db_crabdb_t;
typedef struct fossil_db_crabdb_value_s  fossil_db_crabdb_value_t;
typedef struct fossil_db_crabdb_result_s fossil_db_crabdb_result_t;


/* ============================================================
 * Built-in Type Identifiers
 * ============================================================
 *
 * These are identifiers rather than enums.
 *
 * Integer:
 *     i8, i16, i32, i64
 *     u8, u16, u32, u64
 *
 * Numeric:
 *     f32, f64
 *
 * Representation:
 *     hex, oct, bin
 *
 * General:
 *     cstr, char, bool
 *
 * Semantic:
 *     size, datetime, duration
 *
 * Dynamic:
 *     any, null
 *
 * ============================================================ */

#define FOSSIL_DB_CRABDB_TYPE_I8        "i8"
#define FOSSIL_DB_CRABDB_TYPE_I16       "i16"
#define FOSSIL_DB_CRABDB_TYPE_I32       "i32"
#define FOSSIL_DB_CRABDB_TYPE_I64       "i64"

#define FOSSIL_DB_CRABDB_TYPE_U8        "u8"
#define FOSSIL_DB_CRABDB_TYPE_U16       "u16"
#define FOSSIL_DB_CRABDB_TYPE_U32       "u32"
#define FOSSIL_DB_CRABDB_TYPE_U64       "u64"

#define FOSSIL_DB_CRABDB_TYPE_HEX       "hex"
#define FOSSIL_DB_CRABDB_TYPE_OCT       "oct"
#define FOSSIL_DB_CRABDB_TYPE_BIN       "bin"

#define FOSSIL_DB_CRABDB_TYPE_F32       "f32"
#define FOSSIL_DB_CRABDB_TYPE_F64       "f64"

#define FOSSIL_DB_CRABDB_TYPE_CSTR      "cstr"
#define FOSSIL_DB_CRABDB_TYPE_CHAR      "char"
#define FOSSIL_DB_CRABDB_TYPE_BOOL      "bool"

#define FOSSIL_DB_CRABDB_TYPE_SIZE      "size"
#define FOSSIL_DB_CRABDB_TYPE_DATETIME  "datetime"
#define FOSSIL_DB_CRABDB_TYPE_DURATION  "duration"

#define FOSSIL_DB_CRABDB_TYPE_ANY       "any"
#define FOSSIL_DB_CRABDB_TYPE_NULL      "null"


/* ============================================================
 * Common Identifiers
 * ============================================================ */

#define FOSSIL_DB_CRABDB_ORDER_ASC      "asc"
#define FOSSIL_DB_CRABDB_ORDER_DESC     "desc"

#define FOSSIL_DB_CRABDB_OPTION_TRUE    "true"
#define FOSSIL_DB_CRABDB_OPTION_FALSE   "false"


/* ============================================================
 * Status
 * ============================================================ */

#define FOSSIL_DB_CRABDB_OK             0
#define FOSSIL_DB_CRABDB_ERROR          -1
#define FOSSIL_DB_CRABDB_INVALID        -2
#define FOSSIL_DB_CRABDB_NOT_FOUND      -3
#define FOSSIL_DB_CRABDB_EXISTS         -4
#define FOSSIL_DB_CRABDB_TYPE_ERROR     -5
#define FOSSIL_DB_CRABDB_CONSTRAINT     -6
#define FOSSIL_DB_CRABDB_SCHEMA         -7
#define FOSSIL_DB_CRABDB_TRANSACTION    -8
#define FOSSIL_DB_CRABDB_STORAGE        -9
#define FOSSIL_DB_CRABDB_IO             -10
#define FOSSIL_DB_CRABDB_BUSY           -11
#define FOSSIL_DB_CRABDB_LOCKED         -12
#define FOSSIL_DB_CRABDB_UNSUPPORTED    -13


/* ============================================================
 * Database Lifecycle
 * ============================================================ */

/**
 * @brief Creates a new persistent CrabDB database.
 *
 * The database is stored as a .crab file.
 */
int fossil_db_crabdb_create(
    fossil_db_crabdb_t **db,
    const char *path
);

/**
 * @brief Opens an existing CrabDB database.
 */
int fossil_db_crabdb_open(
    fossil_db_crabdb_t **db,
    const char *path
);

/**
 * @brief Closes a CrabDB database.
 */
int fossil_db_crabdb_close(
    fossil_db_crabdb_t *db
);

/**
 * @brief Flushes pending database operations.
 */
int fossil_db_crabdb_flush(
    fossil_db_crabdb_t *db
);

/**
 * @brief Synchronizes persistent database state.
 */
int fossil_db_crabdb_sync(
    fossil_db_crabdb_t *db
);

/**
 * @brief Checks whether a database file exists.
 */
bool fossil_db_crabdb_exists(
    const char *path
);

/**
 * @brief Removes a CrabDB database file.
 */
int fossil_db_crabdb_remove(
    const char *path
);


/* ============================================================
 * Type Information
 * ============================================================ */

/**
 * @brief Checks whether a type identifier is supported.
 */
bool fossil_db_crabdb_type_supported(
    const char *type_id
);

/**
 * @brief Returns the native size of a type.
 *
 * Returns 0 for variable-sized or unknown types.
 */
size_t fossil_db_crabdb_type_sizeof(
    const char *type_id
);

/**
 * @brief Returns whether a type is numeric.
 */
bool fossil_db_crabdb_type_numeric(
    const char *type_id
);


/* ============================================================
 * Value API
 * ============================================================ */

/**
 * @brief Initializes a database value.
 */
int fossil_db_crabdb_value_init(
    fossil_db_crabdb_value_t *value
);

/**
 * @brief Clears a database value.
 */
void fossil_db_crabdb_value_clear(
    fossil_db_crabdb_value_t *value
);

/**
 * @brief Sets a value using a CrabDB type identifier.
 */
int fossil_db_crabdb_value_set(
    fossil_db_crabdb_value_t *value,
    const char *type_id,
    const void *data,
    size_t size
);

/**
 * @brief Retrieves the type identifier of a value.
 */
const char *fossil_db_crabdb_value_type(
    const fossil_db_crabdb_value_t *value
);

/**
 * @brief Retrieves the raw value data.
 */
const void *fossil_db_crabdb_value_data(
    const fossil_db_crabdb_value_t *value
);

/**
 * @brief Retrieves the value size.
 */
size_t fossil_db_crabdb_value_size(
    const fossil_db_crabdb_value_t *value
);

/**
 * @brief Converts a value to another CrabDB type.
 */
int fossil_db_crabdb_value_convert(
    fossil_db_crabdb_value_t *value,
    const char *type_id
);


/* ============================================================
 * Table Operations
 * ============================================================ */

/**
 * @brief Creates a table.
 */
int fossil_db_crabdb_table_create(
    fossil_db_crabdb_t *db,
    const char *table
);

/**
 * @brief Drops a table.
 */
int fossil_db_crabdb_table_drop(
    fossil_db_crabdb_t *db,
    const char *table
);

/**
 * @brief Checks whether a table exists.
 */
bool fossil_db_crabdb_table_exists(
    fossil_db_crabdb_t *db,
    const char *table
);

/**
 * @brief Renames a table.
 */
int fossil_db_crabdb_table_rename(
    fossil_db_crabdb_t *db,
    const char *table,
    const char *name
);


/* ============================================================
 * Column Operations
 * ============================================================ */

/**
 * @brief Adds a column to a table.
 */
int fossil_db_crabdb_column_add(
    fossil_db_crabdb_t *db,
    const char *table,
    const char *column,
    const char *type_id
);

/**
 * @brief Removes a column from a table.
 */
int fossil_db_crabdb_column_drop(
    fossil_db_crabdb_t *db,
    const char *table,
    const char *column
);

/**
 * @brief Renames a column.
 */
int fossil_db_crabdb_column_rename(
    fossil_db_crabdb_t *db,
    const char *table,
    const char *column,
    const char *name
);

/**
 * @brief Checks whether a column exists.
 */
bool fossil_db_crabdb_column_exists(
    fossil_db_crabdb_t *db,
    const char *table,
    const char *column
);


/* ============================================================
 * CRUD Operations
 * ============================================================ */

/**
 * @brief Inserts a record into a table.
 */
int fossil_db_crabdb_insert(
    fossil_db_crabdb_t *db,
    const char *table,
    const char *columns,
    const fossil_db_crabdb_value_t *values,
    size_t count
);

/**
 * @brief Retrieves a record by its identifier.
 */
int fossil_db_crabdb_get(
    fossil_db_crabdb_t *db,
    const char *table,
    const char *id,
    fossil_db_crabdb_result_t **result
);

/**
 * @brief Finds records matching an operation.
 */
int fossil_db_crabdb_find(
    fossil_db_crabdb_t *db,
    const char *table,
    const char *column,
    const char *operator_id,
    const fossil_db_crabdb_value_t *value,
    fossil_db_crabdb_result_t **result
);

/**
 * @brief Updates records matching an operation.
 */
int fossil_db_crabdb_update(
    fossil_db_crabdb_t *db,
    const char *table,
    const char *column,
    const fossil_db_crabdb_value_t *value,
    const char *where_column,
    const char *operator_id,
    const fossil_db_crabdb_value_t *where_value
);

/**
 * @brief Deletes records matching an operation.
 */
int fossil_db_crabdb_delete(
    fossil_db_crabdb_t *db,
    const char *table,
    const char *column,
    const char *operator_id,
    const fossil_db_crabdb_value_t *value
);


/* ============================================================
 * Bulk Operations
 * ============================================================ */

/**
 * @brief Inserts multiple records.
 */
int fossil_db_crabdb_insert_many(
    fossil_db_crabdb_t *db,
    const char *table,
    const fossil_db_crabdb_value_t *values,
    size_t rows,
    size_t columns
);


/* ============================================================
 * Operators
 * ============================================================ */

/**
 * @brief Checks whether an operator identifier is supported.
 *
 * Core operators:
 *
 *     eq
 *     neq
 *     lt
 *     lte
 *     gt
 *     gte
 *     between
 *     in
 *     not_in
 *     like
 *     not_like
 *     is_null
 *     not_null
 *     contains
 *     starts_with
 *     ends_with
 */
bool fossil_db_crabdb_operator_supported(
    const char *operator_id
);


/* ============================================================
 * Ordering
 * ============================================================ */

/**
 * @brief Sets ordering for a result.
 */
int fossil_db_crabdb_order(
    fossil_db_crabdb_result_t *result,
    const char *column,
    const char *order_id
);

/**
 * @brief Limits the number of records in a result.
 */
int fossil_db_crabdb_limit(
    fossil_db_crabdb_result_t *result,
    size_t limit
);

/**
 * @brief Sets the result offset.
 */
int fossil_db_crabdb_offset(
    fossil_db_crabdb_result_t *result,
    size_t offset
);


/* ============================================================
 * Result Operations
 * ============================================================ */

/**
 * @brief Returns the number of records in a result.
 */
size_t fossil_db_crabdb_result_count(
    const fossil_db_crabdb_result_t *result
);

/**
 * @brief Retrieves a value from a result.
 */
int fossil_db_crabdb_result_value(
    const fossil_db_crabdb_result_t *result,
    size_t row,
    const char *column,
    fossil_db_crabdb_value_t *value
);

/**
 * @brief Releases a result.
 */
void fossil_db_crabdb_result_free(
    fossil_db_crabdb_result_t *result
);


/* ============================================================
 * Index Operations
 * ============================================================ */

/**
 * @brief Creates an index.
 */
int fossil_db_crabdb_index_create(
    fossil_db_crabdb_t *db,
    const char *table,
    const char *column
);

/**
 * @brief Creates an index with configuration.
 *
 * Example configuration:
 *
 *     "unique", true
 */
int fossil_db_crabdb_index_create_config(
    fossil_db_crabdb_t *db,
    const char *table,
    const char *column,
    const char *option,
    bool value
);

/**
 * @brief Drops an index.
 */
int fossil_db_crabdb_index_drop(
    fossil_db_crabdb_t *db,
    const char *table,
    const char *column
);

/**
 * @brief Checks whether an index exists.
 */
bool fossil_db_crabdb_index_exists(
    fossil_db_crabdb_t *db,
    const char *table,
    const char *column
);


/* ============================================================
 * Transactions
 * ============================================================ */

/**
 * @brief Begins a transaction.
 */
int fossil_db_crabdb_transaction_begin(
    fossil_db_crabdb_t *db
);

/**
 * @brief Commits the current transaction.
 */
int fossil_db_crabdb_transaction_commit(
    fossil_db_crabdb_t *db
);

/**
 * @brief Rolls back the current transaction.
 */
int fossil_db_crabdb_transaction_rollback(
    fossil_db_crabdb_t *db
);

/**
 * @brief Creates a transaction savepoint.
 */
int fossil_db_crabdb_transaction_savepoint(
    fossil_db_crabdb_t *db,
    const char *name
);

/**
 * @brief Releases a transaction savepoint.
 */
int fossil_db_crabdb_transaction_release(
    fossil_db_crabdb_t *db,
    const char *name
);

/**
 * @brief Rolls back to a transaction savepoint.
 */
int fossil_db_crabdb_transaction_rollback_to(
    fossil_db_crabdb_t *db,
    const char *name
);


/* ============================================================
 * Introspection
 * ============================================================ */

/**
 * @brief Returns database information.
 */
int fossil_db_crabdb_info(
    fossil_db_crabdb_t *db,
    fossil_db_crabdb_result_t **result
);

/**
 * @brief Returns table information.
 */
int fossil_db_crabdb_table_info(
    fossil_db_crabdb_t *db,
    const char *table,
    fossil_db_crabdb_result_t **result
);

/**
 * @brief Returns column information.
 */
int fossil_db_crabdb_column_info(
    fossil_db_crabdb_t *db,
    const char *table,
    const char *column,
    fossil_db_crabdb_result_t **result
);

/**
 * @brief Returns index information.
 */
int fossil_db_crabdb_index_info(
    fossil_db_crabdb_t *db,
    const char *table,
    fossil_db_crabdb_result_t **result
);


/* ============================================================
 * Error Handling
 * ============================================================ */

/**
 * @brief Returns the last CrabDB error message.
 */
const char *fossil_db_crabdb_error(
    const fossil_db_crabdb_t *db
);

/**
 * @brief Returns a human-readable status description.
 */
const char *fossil_db_crabdb_status_string(
    int status
);


#ifdef __cplusplus
}
#endif

#endif /* FOSSIL_DB_CRABDB_H */
