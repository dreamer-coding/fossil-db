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
    bool corrupted;

    /* Versioning */
    uint64_t version;              /* current working version */
    uint64_t last_commit_version;  /* last finalized commit */
    char last_commit_hash[64];     /* hash of last commit */

    /* Integrity */
    char root_hash[64];            /* global database state hash */
    uint64_t integrity_version;    /* last verified version */

    /* Stats */
    size_t entry_count;
    size_t relation_count;

    /* Error state */
    char last_error[FOSSIL_DB_MAX_ERROR];
    uint32_t last_error_code;

    /* Internal engine */
    void *internal; /* storage engine, indexes, WAL/append log, DSL runtime */

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
int fossil_db_database_create(const char *path, const char *name);

/**
 * Open an existing database
 *
 * @param db Database handle
 * @param path Path to the database file
 * @return 0 on success, non-zero on error
 */
int fossil_db_database_open(fossil_db_t *db, const char *path);

/**
 * Close database and release resources
 *
 * @param db Database handle
 * @return 0 on success, non-zero on error
 */
int fossil_db_database_close(fossil_db_t *db);

/**
 * Delete database from disk
 *
 * @param path Path to the database file to delete
 * @return 0 on success, non-zero on error
 */
int fossil_db_database_delete(const char *path);

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
int fossil_db_database_insert(
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
int fossil_db_database_get(
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
int fossil_db_database_update(
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
int fossil_db_database_remove(
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
int fossil_db_database_insert_sub(
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
int fossil_db_database_get_sub(
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
int fossil_db_database_link(
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
int fossil_db_database_unlink(
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
int fossil_db_database_query(
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
int fossil_db_database_commit(
    fossil_db_t *db,
    const char *message);

/**
 * Checkout a specific version of the database
 *
 * @param db Database handle
 * @param version Version identifier
 * @return 0 on success, non-zero on error
 */
int fossil_db_database_checkout(
    fossil_db_t *db,
    const char *version);

/**
 * Retrieve the commit log for the database
 *
 * @param db Database handle
 * @return 0 on success, non-zero on error
 */
int fossil_db_database_log(
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
int fossil_db_database_verify(fossil_db_t *db);

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
int fossil_db_database_compact(fossil_db_t *db);

/**
 * Backup the database to a specified path
 *
 * @param db Database handle
 * @param backup_path Path to save the backup file
 * @return 0 on success, non-zero on error
 */
int fossil_db_database_backup(
    fossil_db_t *db,
    const char *backup_path);

/**
 * Restore the database from a backup file
 *
 * @param db Database handle
 * @param backup_path Path to the backup file to restore from
 * @return 0 on success, non-zero on error
 */
int fossil_db_database_restore(
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
int fossil_db_database_export(
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
int fossil_db_database_import(
    fossil_db_t *db,
    const char *format,
    const char *input_path);


/*
------------------------------------------------------------
Hash / Data Integrity
------------------------------------------------------------
*/

/**
 * Compute a hash for arbitrary data
 *
 * @param data Input data
 * @param out_hash Output buffer (must be large enough)
 * @return 0 on success, non-zero on error
 */
int fossil_db_database_hash(
    const char *data,
    char *out_hash);

/**
 * Retrieve stored hash for an entry
 *
 * @param db Database handle
 * @param id Entry ID
 * @param out_hash Output buffer
 * @return 0 on success, non-zero on error
 */
int fossil_db_database_get_hash(
    fossil_db_t *db,
    const char *id,
    char *out_hash);

/**
 * Verify integrity of a specific entry
 *
 * @param db Database handle
 * @param id Entry ID
 * @return 0 if valid, non-zero if corrupted
 */
int fossil_db_database_verify_entry(
    fossil_db_t *db,
    const char *id);

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
const char *fossil_db_database_last_error(fossil_db_t *db);

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
            return fossil_db_database_insert(&db_, id.c_str(), data.c_str()) == 0;
        }

        bool get(const std::string &id, std::string &out_data)
        {
            char *data = nullptr;
            if (fossil_db_database_get(&db_, id.c_str(), &data) == 0)
            {
                out_data = data;
                free(data);
                return true;
            }
            return false;
        }

        bool update(const std::string &id, const std::string &data)
        {
            return fossil_db_database_update(&db_, id.c_str(), data.c_str()) == 0;
        }

        bool remove(const std::string &id)
        {
            return fossil_db_database_remove(&db_, id.c_str()) == 0;
        }

    private:
        fossil_db_t db_;
    };

}

#endif

#endif /* FOSSIL_DB_FRAMEWORK_H */
