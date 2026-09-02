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
#include "fossil/crabdb/crabql.h"

#include <ctype.h>

#include <stdio.h>

#include <stdlib.h>

#include <string.h>

/* ============================================================

 * Internal Constants

 * ============================================================ */

#define CRABQL_INITIAL_CAPACITY 4

#define CRABQL_ERROR_SIZE       256

#define CRABQL_SOURCE_SIZE      4096

#define CRABQL_NAME_SIZE        256

/* ============================================================

 * Internal Structures

 * ============================================================ */

struct fossil_db_crabql_argument_s {

    char *name;

    fossil_db_crabdb_value_t *value;

};

struct fossil_db_crabql_expression_s {

    char *source;

    fossil_db_crabql_status_t status;

    size_t error_line;

    size_t error_column;

    char error[CRABQL_ERROR_SIZE];

};

struct fossil_db_crabql_query_s {

    fossil_db_crabql_query_type_t type;

    char *source;

    char *command;

    char *table;

    char **fields;

    size_t field_count;

    size_t field_capacity;

    char *where;

    char *order_by;

    size_t limit;

    size_t offset;

    bool has_limit;

    bool has_offset;

    fossil_db_crabql_argument_t *arguments;

    size_t argument_count;

    size_t argument_capacity;

    fossil_db_crabql_status_t status;

    size_t error_line;

    size_t error_column;

    char error[CRABQL_ERROR_SIZE];

};

struct fossil_db_crabql_result_s {

    size_t count;

    fossil_db_crabql_status_t status;

};

/*

 * The public header forward-declares fossil_db_crabql_t separately

 * from fossil_db_crabql_query_t. The query object is the public

 * object currently used by the API, so no separate database context

 * is required here.

 */

/* ============================================================

 * Internal Utilities

 * ============================================================ */

static char *

crabql_strdup(const char *source)

{

    size_t length;

    char *copy;

    if (source == NULL) {

        return NULL;

    }

    length = strlen(source);

    copy = (char *)malloc(length + 1);

    if (copy == NULL) {

        return NULL;

    }

    memcpy(copy, source, length + 1);

    return copy;

}

static void

crabql_set_error(

    fossil_db_crabql_query_t *query,

    fossil_db_crabql_status_t status,

    size_t line,

    size_t column,

    const char *message

)

{

    if (query == NULL) {

        return;

    }

    query->status = status;

    query->error_line = line;

    query->error_column = column;

    if (message == NULL) {

        query->error[0] = '\0';

        return;

    }

    snprintf(

        query->error,

        sizeof(query->error),

        "%s",

        message

    );

}

static void

crabql_expression_error(

    fossil_db_crabql_expression_t *expression,

    fossil_db_crabql_status_t status,

    size_t line,

    size_t column,

    const char *message

)

{

    if (expression == NULL) {

        return;

    }

    expression->status = status;

    expression->error_line = line;

    expression->error_column = column;

    if (message == NULL) {

        expression->error[0] = '\0';

        return;

    }

    snprintf(

        expression->error,

        sizeof(expression->error),

        "%s",

        message

    );

}

static int

crabql_strcasecmp(

    const char *a,

    const char *b

)

{

    unsigned char ca;

    unsigned char cb;

    if (a == NULL || b == NULL) {

        return -1;

    }

    while (*a != '\0' && *b != '\0') {

        ca = (unsigned char)tolower((unsigned char)*a);

        cb = (unsigned char)tolower((unsigned char)*b);

        if (ca != cb) {

            return (ca > cb) ? 1 : -1;

        }

        ++a;

        ++b;

    }

    if (*a == *b) {

        return 0;

    }

    return (*a != '\0') ? 1 : -1;

}

static bool

crabql_is_space(char c)

{

    return isspace((unsigned char)c) != 0;

}

static const char *

crabql_skip_space(const char *text)

{

    if (text == NULL) {

        return NULL;

    }

    while (*text != '\0' && crabql_is_space(*text)) {

        ++text;

    }

    return text;

}

static bool

crabql_keyword_boundary(

    const char *position,

    const char *keyword

)

{

    size_t length;

    if (position == NULL || keyword == NULL) {

        return false;

    }

    length = strlen(keyword);

    if (strncmp(position, keyword, length) != 0 &&

        strncasecmp(position, keyword, length) != 0) {

        return false;

    }

    if (position > keyword) {

        char previous = position[-1];

        if (isalnum((unsigned char)previous) ||

            previous == '_') {

            return false;

        }

    }

    if (position[length] != '\0' &&

        (isalnum((unsigned char)position[length]) ||

         position[length] == '_')) {

        return false;

    }

    return true;

}

/*

 * Portable case-insensitive substring search.

 */

static const char *

crabql_find_keyword(

    const char *source,

    const char *keyword

)

{

    size_t length;

    const char *cursor;

    if (source == NULL || keyword == NULL) {

        return NULL;

    }

    length = strlen(keyword);

    if (length == 0) {

        return source;

    }

    cursor = source;

    while (*cursor != '\0') {

        if (crabql_keyword_boundary(cursor, keyword)) {

            return cursor;

        }

        ++cursor;

    }

    return NULL;

}

static char *

crabql_trim_copy(

    const char *begin,

    const char *end

)

{

    while (begin < end && crabql_is_space(*begin)) {

        ++begin;

    }

    while (end > begin && crabql_is_space(end[-1])) {

        --end;

    }

    {

        size_t length = (size_t)(end - begin);

        char *result = (char *)malloc(length + 1);

        if (result == NULL) {

            return NULL;

        }

        memcpy(result, begin, length);

        result[length] = '\0';

        return result;

    }

}

static void

crabql_strip_semicolon(char *text)

{

    size_t length;

    if (text == NULL) {

        return;

    }

    length = strlen(text);

    while (length > 0 &&

           crabql_is_space(text[length - 1])) {

        text[--length] = '\0';

    }

    if (length > 0 && text[length - 1] == ';') {

        text[length - 1] = '\0';

    }

}

/* ============================================================

 * Dynamic Field Storage

 * ============================================================ */

static fossil_db_crabql_status_t

crabql_add_field(

    fossil_db_crabql_query_t *query,

    const char *field

)

{

    char **fields;

    char *copy;

    if (query == NULL || field == NULL || field[0] == '\0') {

        return FOSSIL_DB_CRABQL_INVALID_ARGUMENT;

    }

    if (query->field_count == query->field_capacity) {

        size_t capacity =

            query->field_capacity == 0

                ? CRABQL_INITIAL_CAPACITY

                : query->field_capacity * 2;

        fields = (char **)realloc(

            query->fields,

            capacity * sizeof(char *)

        );

        if (fields == NULL) {

            return FOSSIL_DB_CRABQL_OUT_OF_MEMORY;

        }

        query->fields = fields;

        query->field_capacity = capacity;

    }

    copy = crabql_strdup(field);

    if (copy == NULL) {

        return FOSSIL_DB_CRABQL_OUT_OF_MEMORY;

    }

    query->fields[query->field_count++] = copy;

    return FOSSIL_DB_CRABQL_SUCCESS;

}

static void

crabql_free_fields(

    fossil_db_crabql_query_t *query

)

{

    size_t i;

    if (query == NULL) {

        return;

    }

    for (i = 0; i < query->field_count; ++i) {

        free(query->fields[i]);

    }

    free(query->fields);

    query->fields = NULL;

    query->field_count = 0;

    query->field_capacity = 0;

}

/* ============================================================

 * Arguments

 * ============================================================ */

static fossil_db_crabql_argument_t *

crabql_find_argument(

    fossil_db_crabql_query_t *query,

    const char *name

)

{

    size_t i;

    if (query == NULL || name == NULL) {

        return NULL;

    }

    for (i = 0; i < query->argument_count; ++i) {

        if (strcmp(query->arguments[i].name, name) == 0) {

            return &query->arguments[i];

        }

    }

    return NULL;

}

static void

crabql_destroy_argument(

    fossil_db_crabql_argument_t *argument

)

{

    if (argument == NULL) {

        return;

    }

    free(argument->name);

    /*

     * The CrabDB value remains owned by the caller.

     *

     * This is intentional because fossil_db_crabql_kwarg()

     * accepts a pointer supplied by the caller rather than

     * constructing a new value.

     */

    argument->name = NULL;

    argument->value = NULL;

}

static void

crabql_free_arguments(

    fossil_db_crabql_query_t *query

)

{

    size_t i;

    if (query == NULL) {

        return;

    }

    for (i = 0; i < query->argument_count; ++i) {

        crabql_destroy_argument(&query->arguments[i]);

    }

    free(query->arguments);

    query->arguments = NULL;

    query->argument_count = 0;

    query->argument_capacity = 0;

}

/* ============================================================

 * Query Type Detection

 * ============================================================ */

static fossil_db_crabql_query_type_t

crabql_detect_type(

    const char *source

)

{

    const char *cursor;

    if (source == NULL) {

        return FOSSIL_DB_CRABQL_QUERY_UNKNOWN;

    }

    cursor = crabql_skip_space(source);

    if (crabql_keyword_boundary(cursor, "SELECT")) {

        return FOSSIL_DB_CRABQL_QUERY_SELECT;

    }

    if (crabql_keyword_boundary(cursor, "INSERT")) {

        return FOSSIL_DB_CRABQL_QUERY_INSERT;

    }

    if (crabql_keyword_boundary(cursor, "UPDATE")) {

        return FOSSIL_DB_CRABQL_QUERY_UPDATE;

    }

    if (crabql_keyword_boundary(cursor, "DELETE")) {

        return FOSSIL_DB_CRABQL_QUERY_DELETE;

    }

    if (crabql_keyword_boundary(cursor, "CREATE")) {

        return FOSSIL_DB_CRABQL_QUERY_CREATE;

    }

    if (crabql_keyword_boundary(cursor, "DROP")) {

        return FOSSIL_DB_CRABQL_QUERY_DROP;

    }

    if (crabql_keyword_boundary(cursor, "ALTER")) {

        return FOSSIL_DB_CRABQL_QUERY_ALTER;

    }

    if (crabql_keyword_boundary(cursor, "BEGIN")) {

        return FOSSIL_DB_CRABQL_QUERY_BEGIN;

    }

    if (crabql_keyword_boundary(cursor, "COMMIT")) {

        return FOSSIL_DB_CRABQL_QUERY_COMMIT;

    }

    if (crabql_keyword_boundary(cursor, "ROLLBACK")) {

        return FOSSIL_DB_CRABQL_QUERY_ROLLBACK;

    }

    /*

     * Python-inspired command form:

     *

     *     select(...)

     *     insert(...)

     *     update(...)

     *     delete(...)

     */

    if (crabql_keyword_boundary(cursor, "select")) {

        return FOSSIL_DB_CRABQL_QUERY_SELECT;

    }

    if (crabql_keyword_boundary(cursor, "insert")) {

        return FOSSIL_DB_CRABQL_QUERY_INSERT;

    }

    if (crabql_keyword_boundary(cursor, "update")) {

        return FOSSIL_DB_CRABQL_QUERY_UPDATE;

    }

    if (crabql_keyword_boundary(cursor, "delete")) {

        return FOSSIL_DB_CRABQL_QUERY_DELETE;

    }

    if (crabql_keyword_boundary(cursor, "create")) {

        return FOSSIL_DB_CRABQL_QUERY_CREATE;

    }

    if (crabql_keyword_boundary(cursor, "drop")) {

        return FOSSIL_DB_CRABQL_QUERY_DROP;

    }

    if (crabql_keyword_boundary(cursor, "alter")) {

        return FOSSIL_DB_CRABQL_QUERY_ALTER;

    }

    if (crabql_keyword_boundary(cursor, "begin")) {

        return FOSSIL_DB_CRABQL_QUERY_BEGIN;

    }

    if (crabql_keyword_boundary(cursor, "commit")) {

        return FOSSIL_DB_CRABQL_QUERY_COMMIT;

    }

    if (crabql_keyword_boundary(cursor, "rollback")) {

        return FOSSIL_DB_CRABQL_QUERY_ROLLBACK;

    }

    return FOSSIL_DB_CRABQL_QUERY_UNKNOWN;

}

/* ============================================================

 * SELECT Parsing

 * ============================================================ */

static fossil_db_crabql_status_t

crabql_parse_select(

    fossil_db_crabql_query_t *query

)

{

    const char *source;

    const char *from;

    const char *where;

    const char *order;

    const char *limit;

    const char *offset;

    if (query == NULL || query->source == NULL) {

        return FOSSIL_DB_CRABQL_INVALID_ARGUMENT;

    }

    source = query->source;

    from = crabql_find_keyword(source, "FROM");

    if (from == NULL) {

        crabql_set_error(

            query,

            FOSSIL_DB_CRABQL_SYNTAX_ERROR,

            1,

            1,

            "SELECT query requires FROM."

        );

        return FOSSIL_DB_CRABQL_SYNTAX_ERROR;

    }

    {

        const char *select_begin =

            source + strlen("SELECT");

        char *fields =

            crabql_trim_copy(

                select_begin,

                from

            );

        if (fields == NULL) {

            return FOSSIL_DB_CRABQL_OUT_OF_MEMORY;

        }

        if (strcmp(fields, "*") == 0) {

            fossil_db_crabql_status_t status =

                fossil_db_crabql_select_all(query);

            free(fields);

            if (status != FOSSIL_DB_CRABQL_SUCCESS) {

                return status;

            }

        } else {

            char *cursor = fields;

            char *start = fields;

            while (true) {

                if (*cursor == ',' || *cursor == '\0') {

                    char saved = *cursor;

                    char *field;

                    *cursor = '\0';

                    field = crabql_trim_copy(

                        start,

                        cursor

                    );

                    if (field == NULL) {

                        free(fields);

                        return FOSSIL_DB_CRABQL_OUT_OF_MEMORY;

                    }

                    if (field[0] == '\0') {

                        free(field);

                        free(fields);

                        crabql_set_error(

                            query,

                            FOSSIL_DB_CRABQL_SYNTAX_ERROR,

                            1,

                            1,

                            "Empty field in SELECT list."

                        );

                        return FOSSIL_DB_CRABQL_SYNTAX_ERROR;

                    }

                    if (crabql_add_field(query, field) !=

                        FOSSIL_DB_CRABQL_SUCCESS) {

                        free(field);

                        free(fields);

                        return FOSSIL_DB_CRABQL_OUT_OF_MEMORY;

                    }

                    free(field);

                    if (saved == '\0') {

                        break;

                    }

                    start = cursor + 1;

                }

                ++cursor;

            }

        }

        free(fields);

    }

    /*

     * Extract table name.

     */

    from += strlen("FROM");

    from = crabql_skip_space(from);

    where = crabql_find_keyword(from, "WHERE");

    order = crabql_find_keyword(from, "ORDER BY");

    limit = crabql_find_keyword(from, "LIMIT");

    offset = crabql_find_keyword(from, "OFFSET");

    {

        const char *end = from + strlen(from);

        if (where != NULL && where < end) {

            end = where;

        }

        if (order != NULL && order < end) {

            end = order;

        }

        if (limit != NULL && limit < end) {

            end = limit;

        }

        if (offset != NULL && offset < end) {

            end = offset;

        }

        query->table = crabql_trim_copy(from, end);

        if (query->table == NULL) {

            return FOSSIL_DB_CRABQL_OUT_OF_MEMORY;

        }

        if (query->table[0] == '\0') {

            crabql_set_error(

                query,

                FOSSIL_DB_CRABQL_SYNTAX_ERROR,

                1,

                1,

                "SELECT query requires a table."

            );

            return FOSSIL_DB_CRABQL_SYNTAX_ERROR;

        }

    }

    /*

     * WHERE

     */

    if (where != NULL) {

        const char *begin = where + strlen("WHERE");

        const char *end = begin + strlen(begin);

        if (order != NULL && order > begin) {

            end = order;

        }

        if (limit != NULL && limit > begin && limit < end) {

            end = limit;

        }

        if (offset != NULL && offset > begin && offset < end) {

            end = offset;

        }

        query->where = crabql_trim_copy(begin, end);

        if (query->where == NULL) {

            return FOSSIL_DB_CRABQL_OUT_OF_MEMORY;

        }

    }

    /*

     * ORDER BY

     */

    if (order != NULL) {

        const char *begin = order + strlen("ORDER BY");

        const char *end = begin + strlen(begin);

        if (limit != NULL && limit > begin) {

            end = limit;

        }

        if (offset != NULL && offset > begin && offset < end) {

            end = offset;

        }

        query->order_by = crabql_trim_copy(begin, end);

        if (query->order_by == NULL) {

            return FOSSIL_DB_CRABQL_OUT_OF_MEMORY;

        }

    }

    /*

     * LIMIT

     */

    if (limit != NULL) {

        const char *begin =

            crabql_skip_space(

                limit + strlen("LIMIT")

            );

        char *endptr = NULL;

        unsigned long long value;

        value = strtoull(begin, &endptr, 10);

        if (endptr == begin) {

            crabql_set_error(

                query,

                FOSSIL_DB_CRABQL_SYNTAX_ERROR,

                1,

                1,

                "Invalid LIMIT value."

            );

            return FOSSIL_DB_CRABQL_SYNTAX_ERROR;

        }

        query->limit = (size_t)value;

        query->has_limit = true;

    }

    /*

     * OFFSET

     */

    if (offset != NULL) {

        const char *begin =

            crabql_skip_space(

                offset + strlen("OFFSET")

            );

        char *endptr = NULL;

        unsigned long long value;

        value = strtoull(begin, &endptr, 10);

        if (endptr == begin) {

            crabql_set_error(

                query,

                FOSSIL_DB_CRABQL_SYNTAX_ERROR,

                1,

                1,

                "Invalid OFFSET value."

            );

            return FOSSIL_DB_CRABQL_SYNTAX_ERROR;

        }

        query->offset = (size_t)value;

        query->has_offset = true;

    }

    return FOSSIL_DB_CRABQL_SUCCESS;

}

/* ============================================================

 * CREATE / DROP / ALTER Parsing

 * ============================================================ */

static fossil_db_crabql_status_t

crabql_parse_table_command(

    fossil_db_crabql_query_t *query

)

{

    const char *source = query->source;

    const char *cursor;

    cursor = source;

    if (query->type == FOSSIL_DB_CRABQL_QUERY_CREATE) {

        cursor += strlen("CREATE");

        cursor = crabql_skip_space(cursor);

        if (crabql_keyword_boundary(cursor, "TABLE")) {

            cursor += strlen("TABLE");

            cursor = crabql_skip_space(cursor);

        }

    }

    else if (query->type == FOSSIL_DB_CRABQL_QUERY_DROP) {

        cursor += strlen("DROP");

        cursor = crabql_skip_space(cursor);

        if (crabql_keyword_boundary(cursor, "TABLE")) {

            cursor += strlen("TABLE");

            cursor = crabql_skip_space(cursor);

        }

    }

    else if (query->type == FOSSIL_DB_CRABQL_QUERY_ALTER) {

        cursor += strlen("ALTER");

        cursor = crabql_skip_space(cursor);

        if (crabql_keyword_boundary(cursor, "TABLE")) {

            cursor += strlen("TABLE");

            cursor = crabql_skip_space(cursor);

        }

    }

    {

        const char *end = cursor;

        while (*end != '\0' &&

               !crabql_is_space(*end) &&

               *end != ';') {

            ++end;

        }

        query->table = crabql_trim_copy(cursor, end);

        if (query->table == NULL) {

            return FOSSIL_DB_CRABQL_OUT_OF_MEMORY;

        }

        if (query->table[0] == '\0') {

            crabql_set_error(

                query,

                FOSSIL_DB_CRABQL_SYNTAX_ERROR,

                1,

                1,

                "Database command requires a table name."

            );

            return FOSSIL_DB_CRABQL_SYNTAX_ERROR;

        }

    }

    return FOSSIL_DB_CRABQL_SUCCESS;

}

/* ============================================================

 * Query Validation

 * ============================================================ */

static fossil_db_crabql_status_t

crabql_validate_query(

    fossil_db_crabql_query_t *query

)

{

    if (query == NULL) {

        return FOSSIL_DB_CRABQL_INVALID_ARGUMENT;

    }

    if (query->type == FOSSIL_DB_CRABQL_QUERY_UNKNOWN) {

        crabql_set_error(

            query,

            FOSSIL_DB_CRABQL_UNKNOWN_COMMAND,

            1,

            1,

            "Unknown CrabQL command."

        );

        return FOSSIL_DB_CRABQL_UNKNOWN_COMMAND;

    }

    switch (query->type) {

        case FOSSIL_DB_CRABQL_QUERY_SELECT:

            return crabql_parse_select(query);

        case FOSSIL_DB_CRABQL_QUERY_CREATE:

        case FOSSIL_DB_CRABQL_QUERY_DROP:

        case FOSSIL_DB_CRABQL_QUERY_ALTER:

            return crabql_parse_table_command(query);

        case FOSSIL_DB_CRABQL_QUERY_INSERT:

        case FOSSIL_DB_CRABQL_QUERY_UPDATE:

        case FOSSIL_DB_CRABQL_QUERY_DELETE:

            /*

             * These are recognized syntactically, but execution

             * requires the future CrabDB record API.

             */

            return FOSSIL_DB_CRABQL_SUCCESS;

        case FOSSIL_DB_CRABQL_QUERY_BEGIN:

        case FOSSIL_DB_CRABQL_QUERY_COMMIT:

        case FOSSIL_DB_CRABQL_QUERY_ROLLBACK:

            return FOSSIL_DB_CRABQL_SUCCESS;

        default:

            break;

    }

    return FOSSIL_DB_CRABQL_SUCCESS;

}

/* ============================================================

 * Query API

 * ============================================================ */

fossil_db_crabql_status_t

fossil_db_crabql_create(

    fossil_db_crabql_query_t **query

)

{

    fossil_db_crabql_query_t *object;

    if (query == NULL) {

        return FOSSIL_DB_CRABQL_INVALID_ARGUMENT;

    }

    *query = NULL;

    object = (fossil_db_crabql_query_t *)

        calloc(1, sizeof(*object));

    if (object == NULL) {

        return FOSSIL_DB_CRABQL_OUT_OF_MEMORY;

    }

    object->type = FOSSIL_DB_CRABQL_QUERY_UNKNOWN;

    object->status = FOSSIL_DB_CRABQL_SUCCESS;

    *query = object;

    return FOSSIL_DB_CRABQL_SUCCESS;

}

fossil_db_crabql_status_t

fossil_db_crabql_parse(

    fossil_db_crabql_query_t **query,

    const char *source

)

{

    fossil_db_crabql_status_t status;

    if (query == NULL || source == NULL) {

        return FOSSIL_DB_CRABQL_INVALID_ARGUMENT;

    }

    status = fossil_db_crabql_create(query);

    if (status != FOSSIL_DB_CRABQL_SUCCESS) {

        return status;

    }

    (*query)->source = crabql_strdup(source);

    if ((*query)->source == NULL) {

        fossil_db_crabql_destroy(*query);

        *query = NULL;

        return FOSSIL_DB_CRABQL_OUT_OF_MEMORY;

    }

    crabql_strip_semicolon((*query)->source);

    (*query)->type =

        crabql_detect_type((*query)->source);

    status = crabql_validate_query(*query);

    if (status != FOSSIL_DB_CRABQL_SUCCESS) {

        return status;

    }

    return FOSSIL_DB_CRABQL_SUCCESS;

}

void

fossil_db_crabql_destroy(

    fossil_db_crabql_query_t *query

)

{

    if (query == NULL) {

        return;

    }

    free(query->source);

    free(query->command);

    free(query->table);

    free(query->where);

    free(query->order_by);

    crabql_free_fields(query);

    crabql_free_arguments(query);

    free(query);

}

fossil_db_crabql_query_type_t

fossil_db_crabql_query_type(

    const fossil_db_crabql_query_t *query

)

{

    if (query == NULL) {

        return FOSSIL_DB_CRABQL_QUERY_UNKNOWN;

    }

    return query->type;

}

const char *

fossil_db_crabql_source(

    const fossil_db_crabql_query_t *query

)

{

    if (query == NULL) {

        return NULL;

    }

    return query->source;

}

/* ============================================================

 * Query Builder

 * ============================================================ */

fossil_db_crabql_status_t

fossil_db_crabql_command(

    fossil_db_crabql_query_t *query,

    const char *command

)

{

    if (query == NULL || command == NULL) {

        return FOSSIL_DB_CRABQL_INVALID_ARGUMENT;

    }

    free(query->command);

    query->command = crabql_strdup(command);

    if (query->command == NULL) {

        return FOSSIL_DB_CRABQL_OUT_OF_MEMORY;

    }

    if (crabql_strcasecmp(command, "select") == 0) {

        query->type = FOSSIL_DB_CRABQL_QUERY_SELECT;

    }

    else if (crabql_strcasecmp(command, "insert") == 0) {

        query->type = FOSSIL_DB_CRABQL_QUERY_INSERT;

    }

    else if (crabql_strcasecmp(command, "update") == 0) {

        query->type = FOSSIL_DB_CRABQL_QUERY_UPDATE;

    }

    else if (crabql_strcasecmp(command, "delete") == 0) {

        query->type = FOSSIL_DB_CRABQL_QUERY_DELETE;

    }

    else if (crabql_strcasecmp(command, "create") == 0) {

        query->type = FOSSIL_DB_CRABQL_QUERY_CREATE;

    }

    else if (crabql_strcasecmp(command, "drop") == 0) {

        query->type = FOSSIL_DB_CRABQL_QUERY_DROP;

    }

    else if (crabql_strcasecmp(command, "alter") == 0) {

        query->type = FOSSIL_DB_CRABQL_QUERY_ALTER;

    }

    else if (crabql_strcasecmp(command, "begin") == 0) {

        query->type = FOSSIL_DB_CRABQL_QUERY_BEGIN;

    }

    else if (crabql_strcasecmp(command, "commit") == 0) {

        query->type = FOSSIL_DB_CRABQL_QUERY_COMMIT;

    }

    else if (crabql_strcasecmp(command, "rollback") == 0) {

        query->type = FOSSIL_DB_CRABQL_QUERY_ROLLBACK;

    }

    else {

        query->type = FOSSIL_DB_CRABQL_QUERY_UNKNOWN;

        crabql_set_error(

            query,

            FOSSIL_DB_CRABQL_UNKNOWN_COMMAND,

            1,

            1,

            "Unknown CrabQL command."

        );

        return FOSSIL_DB_CRABQL_UNKNOWN_COMMAND;

    }

    return FOSSIL_DB_CRABQL_SUCCESS;

}

fossil_db_crabql_status_t

fossil_db_crabql_from(

    fossil_db_crabql_query_t *query,

    const char *table

)

{

    if (query == NULL || table == NULL || table[0] == '\0') {

        return FOSSIL_DB_CRABQL_INVALID_ARGUMENT;

    }

    free(query->table);

    query->table = crabql_strdup(table);

    if (query->table == NULL) {

        return FOSSIL_DB_CRABQL_OUT_OF_MEMORY;

    }

    return FOSSIL_DB_CRABQL_SUCCESS;

}

fossil_db_crabql_status_t

fossil_db_crabql_select(

    fossil_db_crabql_query_t *query,

    const char *field

)

{

    return crabql_add_field(query, field);

}

fossil_db_crabql_status_t

fossil_db_crabql_select_all(

    fossil_db_crabql_query_t *query

)

{

    if (query == NULL) {

        return FOSSIL_DB_CRABQL_INVALID_ARGUMENT;

    }

    crabql_free_fields(query);

    return crabql_add_field(query, "*");

}

fossil_db_crabql_status_t

fossil_db_crabql_where(

    fossil_db_crabql_query_t *query,

    const char *expression

)

{

    if (query == NULL || expression == NULL) {

        return FOSSIL_DB_CRABQL_INVALID_ARGUMENT;

    }

    free(query->where);

    query->where = crabql_strdup(expression);

    if (query->where == NULL) {

        return FOSSIL_DB_CRABQL_OUT_OF_MEMORY;

    }

    return FOSSIL_DB_CRABQL_SUCCESS;

}

fossil_db_crabql_status_t

fossil_db_crabql_order_by(

    fossil_db_crabql_query_t *query,

    const char *field

)

{

    if (query == NULL || field == NULL) {

        return FOSSIL_DB_CRABQL_INVALID_ARGUMENT;

    }

    free(query->order_by);

    query->order_by = crabql_strdup(field);

    if (query->order_by == NULL) {

        return FOSSIL_DB_CRABQL_OUT_OF_MEMORY;

    }

    return FOSSIL_DB_CRABQL_SUCCESS;

}

fossil_db_crabql_status_t

fossil_db_crabql_limit(

    fossil_db_crabql_query_t *query,

    size_t limit

)

{

    if (query == NULL) {

        return FOSSIL_DB_CRABQL_INVALID_ARGUMENT;

    }

    query->limit = limit;

    query->has_limit = true;

    return FOSSIL_DB_CRABQL_SUCCESS;

}

fossil_db_crabql_status_t

fossil_db_crabql_offset(

    fossil_db_crabql_query_t *query,

    size_t offset

)

{

    if (query == NULL) {

        return FOSSIL_DB_CRABQL_INVALID_ARGUMENT;

    }

    query->offset = offset;

    query->has_offset = true;

    return FOSSIL_DB_CRABQL_SUCCESS;

}

/* ============================================================

 * Keyword Arguments

 * ============================================================ */

fossil_db_crabql_status_t

fossil_db_crabql_kwarg(

    fossil_db_crabql_query_t *query,

    const char *name,

    fossil_db_crabdb_value_t *value

)

{

    fossil_db_crabql_argument_t *argument;

    if (query == NULL ||

        name == NULL ||

        name[0] == '\0') {

        return FOSSIL_DB_CRABQL_INVALID_ARGUMENT;

    }

    argument = crabql_find_argument(query, name);

    if (argument != NULL) {

        argument->value = value;

        return FOSSIL_DB_CRABQL_SUCCESS;

    }

    if (query->argument_count == query->argument_capacity) {

        size_t capacity =

            query->argument_capacity == 0

                ? CRABQL_INITIAL_CAPACITY

                : query->argument_capacity * 2;

        fossil_db_crabql_argument_t *arguments;

        arguments = (fossil_db_crabql_argument_t *)

            realloc(

                query->arguments,

                capacity * sizeof(*arguments)

            );

        if (arguments == NULL) {

            return FOSSIL_DB_CRABQL_OUT_OF_MEMORY;

        }

        query->arguments = arguments;

        query->argument_capacity = capacity;

    }

    argument = &query->arguments[query->argument_count];

    memset(argument, 0, sizeof(*argument));

    argument->name = crabql_strdup(name);

    if (argument->name == NULL) {

        return FOSSIL_DB_CRABQL_OUT_OF_MEMORY;

    }

    argument->value = value;

    ++query->argument_count;

    return FOSSIL_DB_CRABQL_SUCCESS;

}

bool

fossil_db_crabql_has_kwarg(

    const fossil_db_crabql_query_t *query,

    const char *name

)

{

    size_t i;

    if (query == NULL || name == NULL) {

        return false;

    }

    for (i = 0; i < query->argument_count; ++i) {

        if (strcmp(query->arguments[i].name, name) == 0) {

            return true;

        }

    }

    return false;

}

fossil_db_crabql_status_t

fossil_db_crabql_remove_kwarg(

    fossil_db_crabql_query_t *query,

    const char *name

)

{

    size_t i;

    if (query == NULL || name == NULL) {

        return FOSSIL_DB_CRABQL_INVALID_ARGUMENT;

    }

    for (i = 0; i < query->argument_count; ++i) {

        if (strcmp(query->arguments[i].name, name) == 0) {

            crabql_destroy_argument(

                &query->arguments[i]

            );

            if (i + 1 < query->argument_count) {

                memmove(

                    &query->arguments[i],

                    &query->arguments[i + 1],

                    (query->argument_count - i - 1) *

                    sizeof(query->arguments[0])

                );

            }

            --query->argument_count;

            return FOSSIL_DB_CRABQL_SUCCESS;

        }

    }

    return FOSSIL_DB_CRABQL_UNKNOWN_ARGUMENT;

}

/* ============================================================

 * Expressions

 * ============================================================ */

fossil_db_crabql_status_t

fossil_db_crabql_expression_create(

    fossil_db_crabql_expression_t **expression,

    const char *source

)

{

    fossil_db_crabql_expression_t *object;

    if (expression == NULL || source == NULL) {

        return FOSSIL_DB_CRABQL_INVALID_ARGUMENT;

    }

    *expression = NULL;

    object = (fossil_db_crabql_expression_t *)

        calloc(1, sizeof(*object));

    if (object == NULL) {

        return FOSSIL_DB_CRABQL_OUT_OF_MEMORY;

    }

    object->source = crabql_strdup(source);

    if (object->source == NULL) {

        free(object);

        return FOSSIL_DB_CRABQL_OUT_OF_MEMORY;

    }

    object->status = FOSSIL_DB_CRABQL_SUCCESS;

    *expression = object;

    return FOSSIL_DB_CRABQL_SUCCESS;

}

void

fossil_db_crabql_expression_destroy(

    fossil_db_crabql_expression_t *expression

)

{

    if (expression == NULL) {

        return;

    }

    free(expression->source);

    free(expression);

}

/*

 * This is intentionally a lightweight validator in 0.1.0.

 *

 * It verifies:

 *

 *   - non-empty expressions

 *   - balanced parentheses

 *   - balanced single/double quotes

 *   - no obvious empty expression

 *

 * Full expression parsing can be expanded later into the

 * CrabQL expression AST.

 */

fossil_db_crabql_status_t

fossil_db_crabql_expression_validate(

    fossil_db_crabql_expression_t *expression

)

{

    const char *cursor;

    int parentheses = 0;

    bool single_quote = false;

    bool double_quote = false;

    if (expression == NULL ||

        expression->source == NULL) {

        return FOSSIL_DB_CRABQL_INVALID_ARGUMENT;

    }

    cursor = expression->source;

    if (*crabql_skip_space(cursor) == '\0') {

        crabql_expression_error(

            expression,

            FOSSIL_DB_CRABQL_SYNTAX_ERROR,

            1,

            1,

            "Expression cannot be empty."

        );

        return FOSSIL_DB_CRABQL_SYNTAX_ERROR;

    }

    while (*cursor != '\0') {

        if (*cursor == '\'' && !double_quote) {

            single_quote = !single_quote;

        }

        else if (*cursor == '"' && !single_quote) {

            double_quote = !double_quote;

        }

        else if (!single_quote && !double_quote) {

            if (*cursor == '(') {

                ++parentheses;

            }

            else if (*cursor == ')') {

                --parentheses;

                if (parentheses < 0) {

                    crabql_expression_error(

                        expression,

                        FOSSIL_DB_CRABQL_SYNTAX_ERROR,

                        1,

                        (size_t)(cursor - expression->source) + 1,

                        "Unexpected closing parenthesis."

                    );

                    return FOSSIL_DB_CRABQL_SYNTAX_ERROR;

                }

            }

        }

        ++cursor;

    }

    if (single_quote || double_quote) {

        crabql_expression_error(

            expression,

            FOSSIL_DB_CRABQL_SYNTAX_ERROR,

            1,

            strlen(expression->source),

            "Unterminated string literal."

        );

        return FOSSIL_DB_CRABQL_SYNTAX_ERROR;

    }

    if (parentheses != 0) {

        crabql_expression_error(

            expression,

            FOSSIL_DB_CRABQL_SYNTAX_ERROR,

            1,

            strlen(expression->source),

            "Unbalanced parentheses."

        );

        return FOSSIL_DB_CRABQL_SYNTAX_ERROR;

    }

    expression->status = FOSSIL_DB_CRABQL_SUCCESS;

    expression->error[0] = '\0';

    return FOSSIL_DB_CRABQL_SUCCESS;

}

const char *

fossil_db_crabql_expression_source(

    const fossil_db_crabql_expression_t *expression

)

{

    if (expression == NULL) {

        return NULL;

    }

    return expression->source;

}

/* ============================================================

 * Execution Helpers

 * ============================================================ */

static fossil_db_crabql_status_t

crabql_database_error(

    fossil_db_crabql_query_t *query,

    fossil_db_crabdb_status_t status

)

{

    const char *error;

    error = fossil_db_crabdb_status_string(status);

    if (error == NULL) {

        error = "CrabDB operation failed.";

    }

    crabql_set_error(

        query,

        FOSSIL_DB_CRABQL_DATABASE_ERROR,

        1,

        1,

        error

    );

    return FOSSIL_DB_CRABQL_DATABASE_ERROR;

}

/*

 * Current CrabDB public API does not expose table schema

 * construction, therefore CREATE TABLE is intentionally kept

 * conservative here.

 *

 * A future CrabDB API can provide:

 *

 *     create_table(table, schema)

 *

 * and CrabQL can dispatch the parsed schema directly.

 */

static fossil_db_crabql_status_t

crabql_execute_create(

    fossil_db_crabql_query_t *query,

    fossil_db_crabdb_t *db

)

{

    (void)query;

    (void)db;

    crabql_set_error(

        query,

        FOSSIL_DB_CRABQL_EXECUTION_ERROR,

        1,

        1,

        "CREATE TABLE requires a CrabDB schema API."

    );

    return FOSSIL_DB_CRABQL_EXECUTION_ERROR;

}

static fossil_db_crabql_status_t

crabql_execute_drop(

    fossil_db_crabql_query_t *query,

    fossil_db_crabdb_t *db

)

{

    fossil_db_crabdb_status_t status;

    status = fossil_db_crabdb_drop_table(

        db,

        query->table

    );

    if (status != FOSSIL_DB_CRABDB_SUCCESS) {

        return crabql_database_error(query, status);

    }

    return FOSSIL_DB_CRABQL_SUCCESS;

}

static fossil_db_crabql_status_t

crabql_execute_alter(

    fossil_db_crabql_query_t *query,

    fossil_db_crabdb_t *db

)

{

    /*

     * Supported form:

     *

     *     ALTER TABLE old RENAME TO new

     */

    const char *rename;

    const char *to;

    rename = crabql_find_keyword(

        query->source,

        "RENAME"

    );

    if (rename == NULL) {

        crabql_set_error(

            query,

            FOSSIL_DB_CRABQL_EXECUTION_ERROR,

            1,

            1,

            "Only ALTER TABLE ... RENAME TO ... is currently supported."

        );

        return FOSSIL_DB_CRABQL_EXECUTION_ERROR;

    }

    to = crabql_find_keyword(rename, "TO");

    if (to == NULL) {

        crabql_set_error(

            query,

            FOSSIL_DB_CRABQL_SYNTAX_ERROR,

            1,

            1,

            "ALTER RENAME requires TO."

        );

        return FOSSIL_DB_CRABQL_SYNTAX_ERROR;

    }

    {

        const char *begin = crabql_skip_space(

            to + strlen("TO")

        );

        const char *end = begin;

        char *new_name;

        fossil_db_crabdb_status_t status;

        while (*end != '\0' &&

               !crabql_is_space(*end) &&

               *end != ';') {

            ++end;

        }

        new_name = crabql_trim_copy(

            begin,

            end

        );

        if (new_name == NULL) {

            return FOSSIL_DB_CRABQL_OUT_OF_MEMORY;

        }

        if (new_name[0] == '\0') {

            free(new_name);

            crabql_set_error(

                query,

                FOSSIL_DB_CRABQL_SYNTAX_ERROR,

                1,

                1,

                "ALTER RENAME requires a new table name."

            );

            return FOSSIL_DB_CRABQL_SYNTAX_ERROR;

        }

        status = fossil_db_crabdb_rename_table(

            db,

            query->table,

            new_name

        );

        free(new_name);

        if (status != FOSSIL_DB_CRABDB_SUCCESS) {

            return crabql_database_error(query, status);

        }

    }

    return FOSSIL_DB_CRABQL_SUCCESS;

}

static fossil_db_crabql_status_t

crabql_execute_transaction(

    fossil_db_crabql_query_t *query,

    fossil_db_crabdb_t *db

)

{

    fossil_db_crabdb_status_t status;

    switch (query->type) {

        case FOSSIL_DB_CRABQL_QUERY_BEGIN:

            status = fossil_db_crabdb_begin(db);

            break;

        case FOSSIL_DB_CRABQL_QUERY_COMMIT:

            status = fossil_db_crabdb_commit(db);

            break;

        case FOSSIL_DB_CRABQL_QUERY_ROLLBACK:

            status = fossil_db_crabdb_rollback(db);

            break;

        default:

            return FOSSIL_DB_CRABQL_INVALID_ARGUMENT;

    }

    if (status != FOSSIL_DB_CRABDB_SUCCESS) {

        return crabql_database_error(query, status);

    }

    return FOSSIL_DB_CRABQL_SUCCESS;

}

static fossil_db_crabql_status_t

crabql_execute_record_query(

    fossil_db_crabql_query_t *query,

    fossil_db_crabdb_t *db

)

{

    (void)db;

    switch (query->type) {

        case FOSSIL_DB_CRABQL_QUERY_SELECT:

            /*

             * Parsing works now. Execution awaits result/record

             * APIs in CrabDB.

             */

            crabql_set_error(

                query,

                FOSSIL_DB_CRABQL_EXECUTION_ERROR,

                1,

                1,

                "SELECT execution requires the CrabDB record/result API."

            );

            return FOSSIL_DB_CRABQL_EXECUTION_ERROR;

        case FOSSIL_DB_CRABQL_QUERY_INSERT:

            crabql_set_error(

                query,

                FOSSIL_DB_CRABQL_EXECUTION_ERROR,

                1,

                1,

                "INSERT execution requires the CrabDB record API."

            );

            return FOSSIL_DB_CRABQL_EXECUTION_ERROR;

        case FOSSIL_DB_CRABQL_QUERY_UPDATE:

            crabql_set_error(

                query,

                FOSSIL_DB_CRABQL_EXECUTION_ERROR,

                1,

                1,

                "UPDATE execution requires the CrabDB record API."

            );

            return FOSSIL_DB_CRABQL_EXECUTION_ERROR;

        case FOSSIL_DB_CRABQL_QUERY_DELETE:

            crabql_set_error(

                query,

                FOSSIL_DB_CRABQL_EXECUTION_ERROR,

                1,

                1,

                "DELETE execution requires the CrabDB record API."

            );

            return FOSSIL_DB_CRABQL_EXECUTION_ERROR;

        default:

            break;

    }

    return FOSSIL_DB_CRABQL_SUCCESS;

}

/* ============================================================

 * Execution

 * ============================================================ */

fossil_db_crabql_status_t

fossil_db_crabql_execute(

    fossil_db_crabql_query_t *query,

    fossil_db_crabdb_t *db,

    fossil_db_crabql_result_t **result

)

{

    fossil_db_crabql_status_t status;

    fossil_db_crabql_result_t *output;

    if (query == NULL || db == NULL) {

        return FOSSIL_DB_CRABQL_INVALID_ARGUMENT;

    }

    if (result != NULL) {

        *result = NULL;

    }

    status = crabql_validate_query(query);

    if (status != FOSSIL_DB_CRABQL_SUCCESS) {

        return status;

    }

    output = (fossil_db_crabql_result_t *)

        calloc(1, sizeof(*output));

    if (output == NULL) {

        crabql_set_error(

            query,

            FOSSIL_DB_CRABQL_OUT_OF_MEMORY,

            1,

            1,

            "Unable to allocate CrabQL result."

        );

        return FOSSIL_DB_CRABQL_OUT_OF_MEMORY;

    }

    switch (query->type) {

        case FOSSIL_DB_CRABQL_QUERY_CREATE:

            status = crabql_execute_create(query, db);

            break;

        case FOSSIL_DB_CRABQL_QUERY_DROP:

            status = crabql_execute_drop(query, db);

            break;

        case FOSSIL_DB_CRABQL_QUERY_ALTER:

            status = crabql_execute_alter(query, db);

            break;

        case FOSSIL_DB_CRABQL_QUERY_BEGIN:

        case FOSSIL_DB_CRABQL_QUERY_COMMIT:

        case FOSSIL_DB_CRABQL_QUERY_ROLLBACK:

            status = crabql_execute_transaction(query, db);

            break;

        case FOSSIL_DB_CRABQL_QUERY_SELECT:

        case FOSSIL_DB_CRABQL_QUERY_INSERT:

        case FOSSIL_DB_CRABQL_QUERY_UPDATE:

        case FOSSIL_DB_CRABQL_QUERY_DELETE:

            status = crabql_execute_record_query(query, db);

            break;

        default:

            crabql_set_error(

                query,

                FOSSIL_DB_CRABQL_UNKNOWN_COMMAND,

                1,

                1,

                "Unknown CrabQL query type."

            );

            status = FOSSIL_DB_CRABQL_UNKNOWN_COMMAND;

            break;

    }

    output->status = status;

    if (status != FOSSIL_DB_CRABQL_SUCCESS) {

        free(output);

        return status;

    }

    if (result != NULL) {

        *result = output;

    } else {

        free(output);

    }

    query->status = FOSSIL_DB_CRABQL_SUCCESS;

    query->error[0] = '\0';

    return FOSSIL_DB_CRABQL_SUCCESS;

}

fossil_db_crabql_status_t

fossil_db_crabql_run(

    fossil_db_crabdb_t *db,

    const char *source,

    fossil_db_crabql_result_t **result

)

{

    fossil_db_crabql_query_t *query;

    fossil_db_crabql_status_t status;

    if (db == NULL || source == NULL) {

        return FOSSIL_DB_CRABQL_INVALID_ARGUMENT;

    }

    if (result != NULL) {

        *result = NULL;

    }

    status = fossil_db_crabql_parse(

        &query,

        source

    );

    if (status != FOSSIL_DB_CRABQL_SUCCESS) {

        if (query != NULL) {

            fossil_db_crabql_destroy(query);

        }

        return status;

    }

    status = fossil_db_crabql_execute(

        query,

        db,

        result

    );

    fossil_db_crabql_destroy(query);

    return status;

}

/* ============================================================

 * Results

 * ============================================================ */

size_t

fossil_db_crabql_result_count(

    const fossil_db_crabql_result_t *result

)

{

    if (result == NULL) {

        return 0;

    }

    return result->count;

}

void

fossil_db_crabql_result_destroy(

    fossil_db_crabql_result_t *result

)

{

    free(result);

}

/* ============================================================

 * Diagnostics

 * ============================================================ */

const char *

fossil_db_crabql_status_string(

    fossil_db_crabql_status_t status

)

{

    switch (status) {

        case FOSSIL_DB_CRABQL_SUCCESS:

            return "success";

        case FOSSIL_DB_CRABQL_ERROR:

            return "error";

        case FOSSIL_DB_CRABQL_INVALID_ARGUMENT:

            return "invalid argument";

        case FOSSIL_DB_CRABQL_OUT_OF_MEMORY:

            return "out of memory";

        case FOSSIL_DB_CRABQL_PARSE_ERROR:

            return "parse error";

        case FOSSIL_DB_CRABQL_SYNTAX_ERROR:

            return "syntax error";

        case FOSSIL_DB_CRABQL_TYPE_ERROR:

            return "type error";

        case FOSSIL_DB_CRABQL_UNKNOWN_COMMAND:

            return "unknown command";

        case FOSSIL_DB_CRABQL_UNKNOWN_IDENTIFIER:

            return "unknown identifier";

        case FOSSIL_DB_CRABQL_UNKNOWN_ARGUMENT:

            return "unknown argument";

        case FOSSIL_DB_CRABQL_INVALID_QUERY:

            return "invalid query";

        case FOSSIL_DB_CRABQL_EXECUTION_ERROR:

            return "execution error";

        case FOSSIL_DB_CRABQL_DATABASE_ERROR:

            return "database error";

        default:

            return "unknown status";

    }

}

const char *
fossil_db_crabql_last_error(const fossil_db_crabql_query_t *query)
{

    if (query == NULL) {
        return NULL;
    }

    return query->error;
}

size_t
fossil_db_crabql_error_line(
    const fossil_db_crabql_query_t *query
)
{

    if (query == NULL) {
        return 0;
    }
    return query->error_line;
}

size_t

fossil_db_crabql_error_column(
    const fossil_db_crabql_query_t *query
)
{

    if (query == NULL) {
        return 0;
    }

    return query->error_column;
}

/* ============================================================
 * Version
 * ============================================================ */

const char *fossil_db_crabql_version(void)
{
    return FOSSIL_DB_CRABQL_VERSION;
}
