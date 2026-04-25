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
#ifndef FOSSIL_DB_DATABASE_H
#define FOSSIL_DB_DATABASE_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C"
{
#endif

    /*
    ------------------------------------------------------------
    Fossil Database Core API
    Unified interface for database operations
    ------------------------------------------------------------
    This API provides a stable entry point into the Fossil DB
    system, abstracting internal domains:

    - database (core engine)
    - storage (persistence)
    - family (relationships)
    - dsl (query execution)
    - media (serialization)
    ------------------------------------------------------------
    */

    /*
    ------------------------------------------------------------
    Constants
    ------------------------------------------------------------
    */

#define FOSSIL_DB_MAX_NAME 128
#define FOSSIL_DB_MAX_PATH 512
#define FOSSIL_DB_MAX_ERROR 256

/*
------------------------------------------------------------
Core Handle
------------------------------------------------------------
*/

typedef struct fossil_db_t
{
    char name[FOSSIL_DB_MAX_NAME];
    char path[FOSSIL_DB_MAX_PATH];

    bool opened;

    uint64_t version;
    size_t entry_count;
    size_t relation_count;

    char last_error[FOSSIL_DB_MAX_ERROR];

    void *internal; /* private engine state */

} fossil_db_t;

/*
------------------------------------------------------------
Lifecycle
------------------------------------------------------------
*/

/**
 * Create a new database
 *
 * @param path Directory path to create the database in
 * @param name Name of the database
 * @return 0 on success, non-zero on error
 */
int fossil_db_database__create(const char *path, const char *name);

/**
 * Open an existing database
 *
 * @param db Database handle
 * @param path Path to the database file
 * @return 0 on success, non-zero on error
 */
int fossil_db_database__open(fossil_db_t *db, const char *path);

/**
 * Close database and release resources
 *
 * @param db Database handle
 * @return 0 on success, non-zero on error
 */
int fossil_db_database__close(fossil_db_t *db);

/**
 * Delete database from disk
 *
 * @param path Path to the database file to delete
 * @return 0 on success, non-zero on error
 */
int fossil_db_database__delete(const char *path);

/*
------------------------------------------------------------
Entry Operations (Core CRUD)
------------------------------------------------------------
*/

/**
 * Insert a new entry into the database
 *
 * @param db Database handle
 * @param id Unique identifier for the entry
 * @param data Arbitrary data blob (json, fson, etc.)
 * @return 0 on success, non-zero on error
 */
int fossil_db_database__insert(
    fossil_db_t *db,
    const char *id,
    const char *data);

/**
 * Retrieve an entry by ID
 *
 * @param db Database handle
 * @param id Unique identifier for the entry
 * @param out_data Output parameter for retrieved data (caller must free)
 * @return 0 on success, non-zero on error
 */
int fossil_db_database__get(
    fossil_db_t *db,
    const char *id,
    char **out_data);

/**
 * Update an existing entry
 *
 * @param db Database handle
 * @param id Unique identifier for the entry
 * @param data New data for the entry
 * @return 0 on success, non-zero on error
 */
int fossil_db_database__update(
    fossil_db_t *db,
    const char *id,
    const char *data);

/**
 * Remove an entry by ID
 *
 * @param db Database handle
 * @param id Unique identifier for the entry
 * @return 0 on success, non-zero on error
 */
int fossil_db_database__remove(
    fossil_db_t *db,
    const char *id);

/*
------------------------------------------------------------
Sub-Entry Operations
------------------------------------------------------------
*/

/**
 * Insert a sub-entry under a parent entry
 *
 * @param db Database handle
 * @param parent_id ID of the parent entry
 * @param sub_id Unique identifier for the sub-entry
 * @param data Arbitrary data blob for the sub-entry
 * @return 0 on success, non-zero on error
 */
int fossil_db_database__insert_sub(
    fossil_db_t *db,
    const char *parent_id,
    const char *sub_id,
    const char *data);

/**
 * Retrieve a sub-entry by parent ID and sub ID
 *
 * @param db Database handle
 * @param parent_id ID of the parent entry
 * @param sub_id Unique identifier for the sub-entry
 * @param out_data Output parameter for retrieved data (caller must free)
 * @return 0 on success, non-zero on error
 */
int fossil_db_database__get_sub(
    fossil_db_t *db,
    const char *parent_id,
    const char *sub_id,
    char **out_data);

/*
------------------------------------------------------------
Relationship (Family / DAG)
------------------------------------------------------------
*/

/**
 * Create a relationship between two entries
 *
 * @param db Database handle
 * @param source_id ID of the source entry
 * @param target_id ID of the target entry
 * @param relation Type of relationship (e.g., "parent", "friend", etc.)
 * @return 0 on success, non-zero on error
 */
int fossil_db_database__link(
    fossil_db_t *db,
    const char *source_id,
    const char *target_id,
    const char *relation);

/**
 * Remove a relationship between two entries
 *
 * @param db Database handle
 * @param source_id ID of the source entry
 * @param target_id ID of the target entry
 * @return 0 on success, non-zero on error
 */
int fossil_db_database__unlink(
    fossil_db_t *db,
    const char *source_id,
    const char *target_id);

/*
------------------------------------------------------------
Query / DSL
------------------------------------------------------------
*/

/**
 * Execute a DSL query (.crab syntax)
 *
 * @param db Database handle
 * @param query DSL query string
 * @param out_result Output parameter for query result (caller must free)
 * @return 0 on success, non-zero on error
 */
int fossil_db_database__query(
    fossil_db_t *db,
    const char *query,
    char **out_result);

/*
------------------------------------------------------------
Search
------------------------------------------------------------
*/

typedef struct fossil_db_database_search_result
{
    char id[FOSSIL_DB_MAX_NAME];
    float score;
    char snippet[128];
} fossil_db_database_search_result;

/**
 * Search for entries matching a specific field and value
 *
 * @param db Database handle
 * @param field Field name to search
 * @param value Value to match
 * @param results Output parameter for search results (caller must free)
 * @param count Output parameter for number of results
 * @return 0 on success, non-zero on error
 */
int fossil_db_database_search(
    fossil_db_t *db,
    const char *field,
    const char *value,
    fossil_db_database_search_result **results,
    size_t *count);

/*
------------------------------------------------------------
Versioning
------------------------------------------------------------
*/

/**
 * Commit current state of the database with a message
 *
 * @param db Database handle
 * @param message Commit message describing the changes
 * @return 0 on success, non-zero on error
 */
int fossil_db_database__commit(
    fossil_db_t *db,
    const char *message);

/**
 * Checkout a specific version of the database
 *
 * @param db Database handle
 * @param version Version identifier
 * @return 0 on success, non-zero on error
 */
int fossil_db_database__checkout(
    fossil_db_t *db,
    const char *version);

/**
 * Retrieve the commit log for the database
 *
 * @param db Database handle
 * @return 0 on success, non-zero on error
 */
int fossil_db_database__log(
    fossil_db_t *db);

/*
------------------------------------------------------------
Integrity
------------------------------------------------------------
*/

/**
 * Verify the integrity of the database
 *
 * @param db Database handle
 * @return 0 if the database is valid, non-zero if corruption is detected
 */
int fossil_db_database__verify(fossil_db_t *db);

/*
------------------------------------------------------------
Meta / Maintenance
------------------------------------------------------------
*/

/**
 * Compact the database to optimize storage and performance
 *
 * @param db Database handle
 * @return 0 on success, non-zero on error
 */
int fossil_db_database__compact(fossil_db_t *db);

/**
 * Backup the database to a specified path
 *
 * @param db Database handle
 * @param backup_path Path to save the backup file
 * @return 0 on success, non-zero on error
 */
int fossil_db_database__backup(
    fossil_db_t *db,
    const char *backup_path);

/**
 * Restore the database from a backup file
 *
 * @param db Database handle
 * @param backup_path Path to the backup file to restore from
 * @return 0 on success, non-zero on error
 */
int fossil_db_database__restore(
    fossil_db_t *db,
    const char *backup_path);

/*
------------------------------------------------------------
Media / Serialization
------------------------------------------------------------
*/

/**
 * Export database to a specific format (json, fson, etc.)
 *
 * @param db Database handle
 * @param format Format to export (e.g., "json", "fson")
 * @param output_path Path to save the exported file
 * @return 0 on success, non-zero on error
 */
int fossil_db_database__export(
    fossil_db_t *db,
    const char *format,
    const char *output_path);

/**
 * Import data from a specific format
 *
 * @param db Database handle
 * @param format Format to import (e.g., "json", "fson")
 * @param input_path Path to the input file
 * @return 0 on success, non-zero on error
 */
int fossil_db_database__import(
    fossil_db_t *db,
    const char *format,
    const char *input_path);

/*
------------------------------------------------------------
Error Handling
------------------------------------------------------------
*/

/**
 * Retrieve the last error message from the database handle
 *
 * @param db Database handle
 * @return Last error message string
 */
const char *fossil_db_database__last_error(fossil_db_t *db);

#ifdef __cplusplus
}
#include <string>
#include <vector>

namespace fossil::db
{

    class Database
    {
    public:
        Database(const std::string &path);
        ~Database();

        // Core CRUD operations
        bool insert(const std::string &id, const std::string &data)
        {
            return fossil_db_database__insert(&db_, id.c_str(), data.c_str()) == 0;
        }

        bool get(const std::string &id, std::string &out_data)
        {
            char *data = nullptr;
            if (fossil_db_database__get(&db_, id.c_str(), &data) == 0)
            {
                out_data = data;
                free(data);
                return true;
            }
            return false;
        }

        bool update(const std::string &id, const std::string &data)
        {
            return fossil_db_database__update(&db_, id.c_str(), data.c_str()) == 0;
        }

        bool remove(const std::string &id)
        {
            return fossil_db_database__remove(&db_, id.c_str()) == 0;
        }

    private:
        fossil_db_t db_;
    };

}

#endif

#endif /* FOSSIL_DB_FRAMEWORK_H */
