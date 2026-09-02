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
#ifndef FOSSIL_DB_CRABQL_H
#define FOSSIL_DB_CRABQL_H

/*
 * CrabQL
 *
 * SQL / Python-inspired query language for Blue Crab.
 *
 * CrabQL provides a lightweight query and expression layer for
 * Fossil Blue Crab databases. It supports SQL-style statements,
 * Python-inspired expressions, keyword-style arguments, and
 * programmatic query construction.
 *
 * Copyright (C) Fossil Logic
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "crabdb.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================
 * Version
 * ============================================================ */

#define FOSSIL_DB_CRABQL_VERSION_MAJOR 0
#define FOSSIL_DB_CRABQL_VERSION_MINOR 1
#define FOSSIL_DB_CRABQL_VERSION_PATCH 0

#define FOSSIL_DB_CRABQL_VERSION \
    "0.1.0"

/* ============================================================
 * Forward Declarations
 * ============================================================ */

typedef struct fossil_db_crabql_s
    fossil_db_crabql_t;

typedef struct fossil_db_crabql_query_s
    fossil_db_crabql_query_t;

typedef struct fossil_db_crabql_result_s
    fossil_db_crabql_result_t;

typedef struct fossil_db_crabql_argument_s
    fossil_db_crabql_argument_t;

typedef struct fossil_db_crabql_expression_s
    fossil_db_crabql_expression_t;

/* ============================================================
 * Status
 * ============================================================ */

typedef enum fossil_db_crabql_status_e {

    FOSSIL_DB_CRABQL_SUCCESS = 0,

    FOSSIL_DB_CRABQL_ERROR,
    FOSSIL_DB_CRABQL_INVALID_ARGUMENT,
    FOSSIL_DB_CRABQL_OUT_OF_MEMORY,

    FOSSIL_DB_CRABQL_PARSE_ERROR,
    FOSSIL_DB_CRABQL_SYNTAX_ERROR,
    FOSSIL_DB_CRABQL_TYPE_ERROR,

    FOSSIL_DB_CRABQL_UNKNOWN_COMMAND,
    FOSSIL_DB_CRABQL_UNKNOWN_IDENTIFIER,
    FOSSIL_DB_CRABQL_UNKNOWN_ARGUMENT,

    FOSSIL_DB_CRABQL_INVALID_QUERY,
    FOSSIL_DB_CRABQL_EXECUTION_ERROR,

    FOSSIL_DB_CRABQL_DATABASE_ERROR

} fossil_db_crabql_status_t;

/* ============================================================
 * Query Language
 * ============================================================ */

typedef enum fossil_db_crabql_query_type_e {

    FOSSIL_DB_CRABQL_QUERY_UNKNOWN = 0,

    FOSSIL_DB_CRABQL_QUERY_SELECT,
    FOSSIL_DB_CRABQL_QUERY_INSERT,
    FOSSIL_DB_CRABQL_QUERY_UPDATE,
    FOSSIL_DB_CRABQL_QUERY_DELETE,

    FOSSIL_DB_CRABQL_QUERY_CREATE,
    FOSSIL_DB_CRABQL_QUERY_DROP,
    FOSSIL_DB_CRABQL_QUERY_ALTER,

    FOSSIL_DB_CRABQL_QUERY_BEGIN,
    FOSSIL_DB_CRABQL_QUERY_COMMIT,
    FOSSIL_DB_CRABQL_QUERY_ROLLBACK

} fossil_db_crabql_query_type_t;

/* ============================================================
 * Operators
 * ============================================================ */

typedef enum fossil_db_crabql_operator_e {

    FOSSIL_DB_CRABQL_OP_NONE = 0,

    FOSSIL_DB_CRABQL_OP_EQ,
    FOSSIL_DB_CRABQL_OP_NE,

    FOSSIL_DB_CRABQL_OP_LT,
    FOSSIL_DB_CRABQL_OP_LTE,

    FOSSIL_DB_CRABQL_OP_GT,
    FOSSIL_DB_CRABQL_OP_GTE,

    FOSSIL_DB_CRABQL_OP_AND,
    FOSSIL_DB_CRABQL_OP_OR,
    FOSSIL_DB_CRABQL_OP_NOT,

    FOSSIL_DB_CRABQL_OP_IN,
    FOSSIL_DB_CRABQL_OP_NOT_IN,

    FOSSIL_DB_CRABQL_OP_LIKE,
    FOSSIL_DB_CRABQL_OP_IS,
    FOSSIL_DB_CRABQL_OP_IS_NOT,

    FOSSIL_DB_CRABQL_OP_ADD,
    FOSSIL_DB_CRABQL_OP_SUB,
    FOSSIL_DB_CRABQL_OP_MUL,
    FOSSIL_DB_CRABQL_OP_DIV,
    FOSSIL_DB_CRABQL_OP_MOD

} fossil_db_crabql_operator_t;

/* ============================================================
 * Query
 * ============================================================ */

/**
 * Creates an empty CrabQL query object.
 */
fossil_db_crabql_status_t
fossil_db_crabql_create(
    fossil_db_crabql_query_t **query
);

/**
 * Creates a query object from a SQL/CrabQL string.
 */
fossil_db_crabql_status_t
fossil_db_crabql_parse(
    fossil_db_crabql_query_t **query,
    const char *source
);

/**
 * Releases a query object.
 */
void
fossil_db_crabql_destroy(
    fossil_db_crabql_query_t *query
);

/**
 * Returns the query type.
 */
fossil_db_crabql_query_type_t
fossil_db_crabql_query_type(
    const fossil_db_crabql_query_t *query
);

/**
 * Returns the original query source.
 */
const char *
fossil_db_crabql_source(
    const fossil_db_crabql_query_t *query
);

/* ============================================================
 * Query Builder
 * ============================================================ */

/**
 * Sets the query command.
 */
fossil_db_crabql_status_t
fossil_db_crabql_command(
    fossil_db_crabql_query_t *query,
    const char *command
);

/**
 * Sets the target table.
 */
fossil_db_crabql_status_t
fossil_db_crabql_from(
    fossil_db_crabql_query_t *query,
    const char *table
);

/**
 * Adds a selected field.
 */
fossil_db_crabql_status_t
fossil_db_crabql_select(
    fossil_db_crabql_query_t *query,
    const char *field
);

/**
 * Selects all fields.
 */
fossil_db_crabql_status_t
fossil_db_crabql_select_all(
    fossil_db_crabql_query_t *query
);

/**
 * Adds a WHERE expression.
 */
fossil_db_crabql_status_t
fossil_db_crabql_where(
    fossil_db_crabql_query_t *query,
    const char *expression
);

/**
 * Adds an ORDER BY expression.
 */
fossil_db_crabql_status_t
fossil_db_crabql_order_by(
    fossil_db_crabql_query_t *query,
    const char *field
);

/**
 * Sets the result limit.
 */
fossil_db_crabql_status_t
fossil_db_crabql_limit(
    fossil_db_crabql_query_t *query,
    size_t limit
);

/**
 * Sets the result offset.
 */
fossil_db_crabql_status_t
fossil_db_crabql_offset(
    fossil_db_crabql_query_t *query,
    size_t offset
);

/* ============================================================
 * Keyword Arguments
 * ============================================================ */

/**
 * Adds a keyword argument to a query.
 *
 * Example:
 *
 *     fossil_db_crabql_kwarg(
 *         query,
 *         "minimum_age",
 *         value
 *     );
 */
fossil_db_crabql_status_t
fossil_db_crabql_kwarg(
    fossil_db_crabql_query_t *query,
    const char *name,
    fossil_db_crabdb_value_t *value
);

/**
 * Checks whether a keyword argument exists.
 */
bool
fossil_db_crabql_has_kwarg(
    const fossil_db_crabql_query_t *query,
    const char *name
);

/**
 * Removes a keyword argument.
 */
fossil_db_crabql_status_t
fossil_db_crabql_remove_kwarg(
    fossil_db_crabql_query_t *query,
    const char *name
);

/* ============================================================
 * Expressions
 * ============================================================ */

/**
 * Creates an expression object.
 */
fossil_db_crabql_status_t
fossil_db_crabql_expression_create(
    fossil_db_crabql_expression_t **expression,
    const char *source
);

/**
 * Releases an expression object.
 */
void
fossil_db_crabql_expression_destroy(
    fossil_db_crabql_expression_t *expression
);

/**
 * Validates an expression.
 */
fossil_db_crabql_status_t
fossil_db_crabql_expression_validate(
    fossil_db_crabql_expression_t *expression
);

/**
 * Returns the expression source.
 */
const char *
fossil_db_crabql_expression_source(
    const fossil_db_crabql_expression_t *expression
);

/* ============================================================
 * Execution
 * ============================================================ */

/**
 * Executes a parsed query against CrabDB.
 */
fossil_db_crabql_status_t
fossil_db_crabql_execute(
    fossil_db_crabql_query_t *query,
    fossil_db_crabdb_t *db,
    fossil_db_crabql_result_t **result
);

/**
 * Executes a query directly from source text.
 */
fossil_db_crabql_status_t
fossil_db_crabql_run(
    fossil_db_crabdb_t *db,
    const char *source,
    fossil_db_crabql_result_t **result
);

/* ============================================================
 * Results
 * ============================================================ */

/**
 * Returns the number of rows in a CrabQL result.
 */
size_t
fossil_db_crabql_result_count(
    const fossil_db_crabql_result_t *result
);

/**
 * Releases a CrabQL result.
 */
void
fossil_db_crabql_result_destroy(
    fossil_db_crabql_result_t *result
);

/* ============================================================
 * Diagnostics
 * ============================================================ */

/**
 * Converts a CrabQL status into readable text.
 */
const char *
fossil_db_crabql_status_string(
    fossil_db_crabql_status_t status
);

/**
 * Returns the last error generated by a query.
 */
const char *
fossil_db_crabql_last_error(
    const fossil_db_crabql_query_t *query
);

/**
 * Returns the line associated with the last error.
 */
size_t
fossil_db_crabql_error_line(
    const fossil_db_crabql_query_t *query
);

/**
 * Returns the column associated with the last error.
 */
size_t
fossil_db_crabql_error_column(
    const fossil_db_crabql_query_t *query
);

/* ============================================================
 * Version
 * ============================================================ */

const char *
fossil_db_crabql_version(void);

#ifdef __cplusplus
}
#endif

#endif /* FOSSIL_DB_CRABQL_H */
