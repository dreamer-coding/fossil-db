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

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>


/*
 * --------------------------------------------------------------------------
 * Version
 * --------------------------------------------------------------------------
 */

#define FOSSIL_DB_CRABDB_VERSION_MAJOR 1
#define FOSSIL_DB_CRABDB_VERSION_MINOR 0
#define FOSSIL_DB_CRABDB_VERSION_PATCH 0


/*
 * --------------------------------------------------------------------------
 * Limits
 * --------------------------------------------------------------------------
 */

#define FOSSIL_DB_CRABDB_MAX_NAME_LENGTH 128
#define FOSSIL_DB_CRABDB_MAX_PATH_LENGTH 512
#define FOSSIL_DB_CRABDB_MAX_TYPE_LENGTH 32


/*
 * --------------------------------------------------------------------------
 * Result codes
 * --------------------------------------------------------------------------
 */

#define FOSSIL_DB_CRABDB_OK              0
#define FOSSIL_DB_CRABDB_ERROR          -1
#define FOSSIL_DB_CRABDB_INVALID        -2
#define FOSSIL_DB_CRABDB_NOT_FOUND      -3
#define FOSSIL_DB_CRABDB_EXISTS         -4
#define FOSSIL_DB_CRABDB_CORRUPT        -5
#define FOSSIL_DB_CRABDB_NOMEM          -6
#define FOSSIL_DB_CRABDB_BUSY           -7
#define FOSSIL_DB_CRABDB_READONLY       -8
#define FOSSIL_DB_CRABDB_CONSTRAINT     -9
#define FOSSIL_DB_CRABDB_TYPE           -10
#define FOSSIL_DB_CRABDB_TRANSACTION    -11
#define FOSSIL_DB_CRABDB_IO             -12
#define FOSSIL_DB_CRABDB_OVERFLOW       -13
#define FOSSIL_DB_CRABDB_UNSUPPORTED    -14


/*
 * --------------------------------------------------------------------------
 * Built-in type identifiers
 * --------------------------------------------------------------------------
 *
 * These remain strings rather than enums.
 *
 * i8, i16, i32, i64
 * u8, u16, u32, u64
 * hex, oct, bin
 * f32, f64
 * cstr, char
 * bool
 * size
 * datetime, duration
 * any, null
 *
 * --------------------------------------------------------------------------
 */


/*
 * --------------------------------------------------------------------------
 * Forward declarations
 * --------------------------------------------------------------------------
 */

typedef struct fossil_db_crabdb fossil_db_crabdb_t;
typedef struct fossil_db_crabdb_value fossil_db_crabdb_value_t;
typedef struct fossil_db_crabdb_record fossil_db_crabdb_record_t;
typedef struct fossil_db_crabdb_result fossil_db_crabdb_result_t;
typedef struct fossil_db_crabdb_schema fossil_db_crabdb_schema_t;
typedef struct fossil_db_crabdb_table fossil_db_crabdb_table_t;
typedef struct fossil_db_crabdb_column fossil_db_crabdb_column_t;


/*
 * --------------------------------------------------------------------------
 * Core handles
 * --------------------------------------------------------------------------
 */

struct fossil_db_crabdb
{
    void* storage;

    char filename[FOSSIL_DB_CRABDB_MAX_PATH_LENGTH];

    size_t page_size;

    bool opened;
    bool readonly;
    bool transaction;
};


/*
 * --------------------------------------------------------------------------
 * Values
 * --------------------------------------------------------------------------
 */

struct fossil_db_crabdb_value
{
    const char* type;

    void* data;

    size_t size;
};


/*
 * --------------------------------------------------------------------------
 * Records
 * --------------------------------------------------------------------------
 */

struct fossil_db_crabdb_record
{
    uint64_t id;

    size_t count;

    fossil_db_crabdb_value_t* values;
};


/*
 * --------------------------------------------------------------------------
 * Result sets
 * --------------------------------------------------------------------------
 */

struct fossil_db_crabdb_result
{
    fossil_db_crabdb_record_t* records;

    size_t count;
};


/*
 * --------------------------------------------------------------------------
 * Database lifecycle
 * --------------------------------------------------------------------------
 */

int fossil_db_crabdb_create(
    const char* filename
);

fossil_db_crabdb_t*
fossil_db_crabdb_open(
    const char* filename
);

fossil_db_crabdb_t*
fossil_db_crabdb_open_configured(
    const char* filename,
    const char* option,
    const char* value
);

int fossil_db_crabdb_close(
    fossil_db_crabdb_t* db
);

int fossil_db_crabdb_destroy(
    const char* filename
);


/*
 * --------------------------------------------------------------------------
 * Database metadata
 * --------------------------------------------------------------------------
 */

const char*
fossil_db_crabdb_filename(
    fossil_db_crabdb_t* db
);

const char*
fossil_db_crabdb_version(
    void
);

size_t
fossil_db_crabdb_page_size(
    fossil_db_crabdb_t* db
);

size_t
fossil_db_crabdb_page_count(
    fossil_db_crabdb_t* db
);

size_t
fossil_db_crabdb_size(
    fossil_db_crabdb_t* db
);


/*
 * --------------------------------------------------------------------------
 * Configuration
 * --------------------------------------------------------------------------
 */

int fossil_db_crabdb_configure(
    fossil_db_crabdb_t* db,
    const char* option,
    const char* value
);

const char*
fossil_db_crabdb_config(
    fossil_db_crabdb_t* db,
    const char* option
);


/*
 * --------------------------------------------------------------------------
 * Schema / tables
 * --------------------------------------------------------------------------
 */

int fossil_db_crabdb_create_table(
    fossil_db_crabdb_t* db,
    const char* name
);

int fossil_db_crabdb_drop_table(
    fossil_db_crabdb_t* db,
    const char* name
);

bool fossil_db_crabdb_table_exists(
    fossil_db_crabdb_t* db,
    const char* name
);

fossil_db_crabdb_table_t*
fossil_db_crabdb_table(
    fossil_db_crabdb_t* db,
    const char* name
);

size_t fossil_db_crabdb_table_count(
    fossil_db_crabdb_t* db
);

const char*
fossil_db_crabdb_table_name(
    fossil_db_crabdb_t* db,
    size_t index
);


/*
 * --------------------------------------------------------------------------
 * Columns
 * --------------------------------------------------------------------------
 */

int fossil_db_crabdb_add_column(
    fossil_db_crabdb_t* db,
    const char* table,
    const char* column,
    const char* type
);

int fossil_db_crabdb_remove_column(
    fossil_db_crabdb_t* db,
    const char* table,
    const char* column
);

bool fossil_db_crabdb_column_exists(
    fossil_db_crabdb_t* db,
    const char* table,
    const char* column
);

fossil_db_crabdb_column_t*
fossil_db_crabdb_column(
    fossil_db_crabdb_t* db,
    const char* table,
    const char* column
);

size_t fossil_db_crabdb_column_count(
    fossil_db_crabdb_t* db,
    const char* table
);

const char*
fossil_db_crabdb_column_name(
    fossil_db_crabdb_t* db,
    const char* table,
    size_t index
);

const char*
fossil_db_crabdb_column_type(
    fossil_db_crabdb_t* db,
    const char* table,
    const char* column
);


/*
 * --------------------------------------------------------------------------
 * Column properties
 * --------------------------------------------------------------------------
 *
 * Property names remain strings.
 *
 * Examples:
 *
 * primary
 * unique
 * nullable
 * indexed
 * auto_increment
 * default
 *
 * --------------------------------------------------------------------------
 */

int fossil_db_crabdb_column_option(
    fossil_db_crabdb_t* db,
    const char* table,
    const char* column,
    const char* option,
    const char* value
);

const char*
fossil_db_crabdb_column_config(
    fossil_db_crabdb_t* db,
    const char* table,
    const char* column,
    const char* option
);


/*
 * --------------------------------------------------------------------------
 * Values
 * --------------------------------------------------------------------------
 */

fossil_db_crabdb_value_t*
fossil_db_crabdb_value_create(
    const char* type,
    const void* data,
    size_t size
);

fossil_db_crabdb_value_t*
fossil_db_crabdb_value_null(
    void
);

fossil_db_crabdb_value_t*
fossil_db_crabdb_value_copy(
    const fossil_db_crabdb_value_t* value
);

void fossil_db_crabdb_value_destroy(
    fossil_db_crabdb_value_t* value
);

int fossil_db_crabdb_value_set(
    fossil_db_crabdb_value_t* value,
    const void* data,
    size_t size
);

int fossil_db_crabdb_value_cast(
    fossil_db_crabdb_value_t* value,
    const char* type
);

const char*
fossil_db_crabdb_value_type(
    const fossil_db_crabdb_value_t* value
);

const void*
fossil_db_crabdb_value_data(
    const fossil_db_crabdb_value_t* value
);

size_t
fossil_db_crabdb_value_size(
    const fossil_db_crabdb_value_t* value
);

bool
fossil_db_crabdb_value_is_null(
    const fossil_db_crabdb_value_t* value
);


/*
 * --------------------------------------------------------------------------
 * Typed value helpers
 * --------------------------------------------------------------------------
 *
 * These avoid forcing callers to manually construct every primitive value.
 * --------------------------------------------------------------------------
 */

fossil_db_crabdb_value_t*
fossil_db_crabdb_value_i8(
    int8_t value
);

fossil_db_crabdb_value_t*
fossil_db_crabdb_value_i16(
    int16_t value
);

fossil_db_crabdb_value_t*
fossil_db_crabdb_value_i32(
    int32_t value
);

fossil_db_crabdb_value_t*
fossil_db_crabdb_value_i64(
    int64_t value
);

fossil_db_crabdb_value_t*
fossil_db_crabdb_value_u8(
    uint8_t value
);

fossil_db_crabdb_value_t*
fossil_db_crabdb_value_u16(
    uint16_t value
);

fossil_db_crabdb_value_t*
fossil_db_crabdb_value_u32(
    uint32_t value
);

fossil_db_crabdb_value_t*
fossil_db_crabdb_value_u64(
    uint64_t value
);

fossil_db_crabdb_value_t*
fossil_db_crabdb_value_f32(
    float value
);

fossil_db_crabdb_value_t*
fossil_db_crabdb_value_f64(
    double value
);

fossil_db_crabdb_value_t*
fossil_db_crabdb_value_cstr(
    const char* value
);

fossil_db_crabdb_value_t*
fossil_db_crabdb_value_char(
    char value
);

fossil_db_crabdb_value_t*
fossil_db_crabdb_value_bool(
    bool value
);

fossil_db_crabdb_value_t*
fossil_db_crabdb_value_size(
    size_t value
);


/*
 * --------------------------------------------------------------------------
 * Typed value access
 * --------------------------------------------------------------------------
 */

int fossil_db_crabdb_value_get_i8(
    const fossil_db_crabdb_value_t* value,
    int8_t* output
);

int fossil_db_crabdb_value_get_i16(
    const fossil_db_crabdb_value_t* value,
    int16_t* output
);

int fossil_db_crabdb_value_get_i32(
    const fossil_db_crabdb_value_t* value,
    int32_t* output
);

int fossil_db_crabdb_value_get_i64(
    const fossil_db_crabdb_value_t* value,
    int64_t* output
);

int fossil_db_crabdb_value_get_u8(
    const fossil_db_crabdb_value_t* value,
    uint8_t* output
);

int fossil_db_crabdb_value_get_u16(
    const fossil_db_crabdb_value_t* value,
    uint16_t* output
);

int fossil_db_crabdb_value_get_u32(
    const fossil_db_crabdb_value_t* value,
    uint32_t* output
);

int fossil_db_crabdb_value_get_u64(
    const fossil_db_crabdb_value_t* value,
    uint64_t* output
);

int fossil_db_crabdb_value_get_f32(
    const fossil_db_crabdb_value_t* value,
    float* output
);

int fossil_db_crabdb_value_get_f64(
    const fossil_db_crabdb_value_t* value,
    double* output
);

int fossil_db_crabdb_value_get_cstr(
    const fossil_db_crabdb_value_t* value,
    const char** output
);

int fossil_db_crabdb_value_get_char(
    const fossil_db_crabdb_value_t* value,
    char* output
);

int fossil_db_crabdb_value_get_bool(
    const fossil_db_crabdb_value_t* value,
    bool* output
);

int fossil_db_crabdb_value_get_size(
    const fossil_db_crabdb_value_t* value,
    size_t* output
);


/*
 * --------------------------------------------------------------------------
 * Records
 * --------------------------------------------------------------------------
 */

fossil_db_crabdb_record_t*
fossil_db_crabdb_record_create(
    size_t count
);

void fossil_db_crabdb_record_destroy(
    fossil_db_crabdb_record_t* record
);

fossil_db_crabdb_record_t*
fossil_db_crabdb_record_copy(
    const fossil_db_crabdb_record_t* record
);

int fossil_db_crabdb_record_set(
    fossil_db_crabdb_record_t* record,
    size_t index,
    fossil_db_crabdb_value_t* value
);

fossil_db_crabdb_value_t*
fossil_db_crabdb_record_get(
    fossil_db_crabdb_record_t* record,
    size_t index
);

int fossil_db_crabdb_record_set_column(
    fossil_db_crabdb_t* db,
    fossil_db_crabdb_record_t* record,
    const char* table,
    const char* column,
    fossil_db_crabdb_value_t* value
);

fossil_db_crabdb_value_t*
fossil_db_crabdb_record_get_column(
    fossil_db_crabdb_t* db,
    fossil_db_crabdb_record_t* record,
    const char* table,
    const char* column
);


/*
 * --------------------------------------------------------------------------
 * CRUD
 * --------------------------------------------------------------------------
 */

int fossil_db_crabdb_insert(
    fossil_db_crabdb_t* db,
    const char* table,
    fossil_db_crabdb_record_t* record
);

int fossil_db_crabdb_insert_many(
    fossil_db_crabdb_t* db,
    const char* table,
    fossil_db_crabdb_record_t** records,
    size_t count
);

int fossil_db_crabdb_select(
    fossil_db_crabdb_t* db,
    const char* table,
    fossil_db_crabdb_result_t** result
);

int fossil_db_crabdb_get(
    fossil_db_crabdb_t* db,
    const char* table,
    uint64_t id,
    fossil_db_crabdb_record_t** record
);

int fossil_db_crabdb_update(
    fossil_db_crabdb_t* db,
    const char* table,
    fossil_db_crabdb_record_t* record
);

int fossil_db_crabdb_delete(
    fossil_db_crabdb_t* db,
    const char* table,
    uint64_t id
);

int fossil_db_crabdb_clear(
    fossil_db_crabdb_t* db,
    const char* table
);


/*
 * --------------------------------------------------------------------------
 * Results
 * --------------------------------------------------------------------------
 */

fossil_db_crabdb_result_t*
fossil_db_crabdb_result_create(
    void
);

void fossil_db_crabdb_result_destroy(
    fossil_db_crabdb_result_t* result
);

size_t fossil_db_crabdb_result_count(
    const fossil_db_crabdb_result_t* result
);

fossil_db_crabdb_record_t*
fossil_db_crabdb_result_get(
    fossil_db_crabdb_result_t* result,
    size_t index
);


/*
 * --------------------------------------------------------------------------
 * Operators
 * --------------------------------------------------------------------------
 */

int fossil_db_crabdb_compare(
    fossil_db_crabdb_value_t* left,
    const char* operation,
    fossil_db_crabdb_value_t* right
);

fossil_db_crabdb_value_t*
fossil_db_crabdb_operation(
    const char* operation,
    fossil_db_crabdb_value_t* left,
    fossil_db_crabdb_value_t* right
);


/*
 * --------------------------------------------------------------------------
 * Transactions
 * --------------------------------------------------------------------------
 */

int fossil_db_crabdb_transaction_begin(
    fossil_db_crabdb_t* db
);

int fossil_db_crabdb_transaction_commit(
    fossil_db_crabdb_t* db
);

int fossil_db_crabdb_transaction_rollback(
    fossil_db_crabdb_t* db
);

int fossil_db_crabdb_transaction_savepoint(
    fossil_db_crabdb_t* db,
    const char* name
);

int fossil_db_crabdb_transaction_release(
    fossil_db_crabdb_t* db,
    const char* name
);

int fossil_db_crabdb_transaction_rollback_to(
    fossil_db_crabdb_t* db,
    const char* name
);


/*
 * --------------------------------------------------------------------------
 * Indexes
 * --------------------------------------------------------------------------
 */

int fossil_db_crabdb_index_create(
    fossil_db_crabdb_t* db,
    const char* table,
    const char* column,
    const char* type
);

int fossil_db_crabdb_index_remove(
    fossil_db_crabdb_t* db,
    const char* table,
    const char* column
);

bool fossil_db_crabdb_index_exists(
    fossil_db_crabdb_t* db,
    const char* table,
    const char* column
);

int fossil_db_crabdb_index_rebuild(
    fossil_db_crabdb_t* db,
    const char* table,
    const char* column
);


/*
 * --------------------------------------------------------------------------
 * Persistence
 * --------------------------------------------------------------------------
 */

int fossil_db_crabdb_sync(
    fossil_db_crabdb_t* db
);

int fossil_db_crabdb_flush(
    fossil_db_crabdb_t* db
);

int fossil_db_crabdb_checkpoint(
    fossil_db_crabdb_t* db
);


/*
 * --------------------------------------------------------------------------
 * Serialization
 * --------------------------------------------------------------------------
 */

int fossil_db_crabdb_value_encode(
    const fossil_db_crabdb_value_t* value,
    void* buffer,
    size_t size,
    size_t* written
);

fossil_db_crabdb_value_t*
fossil_db_crabdb_value_decode(
    const void* buffer,
    size_t size,
    size_t* consumed
);


/*
 * --------------------------------------------------------------------------
 * Maintenance
 * --------------------------------------------------------------------------
 */

int fossil_db_crabdb_check(
    fossil_db_crabdb_t* db
);

int fossil_db_crabdb_vacuum(
    fossil_db_crabdb_t* db
);

int fossil_db_crabdb_recover(
    fossil_db_crabdb_t* db
);

int fossil_db_crabdb_backup(
    fossil_db_crabdb_t* db,
    const char* filename
);


/*
 * --------------------------------------------------------------------------
 * Error information
 * --------------------------------------------------------------------------
 */

int fossil_db_crabdb_error(
    fossil_db_crabdb_t* db
);

const char*
fossil_db_crabdb_error_string(
    int error
);

const char*
fossil_db_crabdb_last_error(
    fossil_db_crabdb_t* db
);


#ifdef __cplusplus
}
#endif

#endif /* FOSSIL_DB_CRABDB_H */
