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
#ifndef FOSSIL_DB_BLUECRAB_H
#define FOSSIL_DB_BLUECRAB_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C"
{
#endif

/*
------------------------------------------------------------
Blue Crab Database
FSON powered database engine
------------------------------------------------------------
Features
- FSON structured storage
- Git-hybrid append model
- DAG relationships
- String ID indexing
- CRUD operations
- AI fuzzy search
- Hash verification
- Multi-entry datasets
- Sub-entry records
------------------------------------------------------------
*/

#define FOSSIL_BLUECRAB_MAX_ID 128
#define FOSSIL_BLUECRAB_HASH_SIZE 64
#define FOSSIL_BLUECRAB_PATH 512

/*
------------------------------------------------------------
Core Types
------------------------------------------------------------
*/

typedef struct fossil_bluecrab_db
{
    char name[FOSSIL_BLUECRAB_MAX_ID];
    char root_path[FOSSIL_BLUECRAB_PATH];
    char meta_path[FOSSIL_BLUECRAB_PATH];
    bool opened;
    size_t entry_count;
    size_t relation_count;
    uint64_t last_commit_version;
    char last_commit_hash[FOSSIL_BLUECRAB_HASH_SIZE];
    char last_error[256];
    void *internal_data;
} fossil_bluecrab_db;

typedef struct fossil_bluecrab_entry
{
    char id[FOSSIL_BLUECRAB_MAX_ID];
    char parent_id[FOSSIL_BLUECRAB_MAX_ID];
    char hash[FOSSIL_BLUECRAB_HASH_SIZE];
    uint64_t version;
    char created_at[32];
    char updated_at[32];
    size_t data_size;
    char *fson_data;
    bool deleted;
    uint32_t flags;
} fossil_bluecrab_entry;

typedef struct fossil_bluecrab_relation
{
    char source_id[FOSSIL_BLUECRAB_MAX_ID];
    char target_id[FOSSIL_BLUECRAB_MAX_ID];
    char relation_type[FOSSIL_BLUECRAB_MAX_ID];
    uint64_t created_version;
    char created_at[32];
    char metadata[128];
} fossil_bluecrab_relation;

typedef struct fossil_bluecrab_search_result
{
    char id[FOSSIL_BLUECRAB_MAX_ID];
    float score;
    char matched_field[FOSSIL_BLUECRAB_MAX_ID];
    char snippet[128];
    uint64_t version;
} fossil_bluecrab_search_result;

/*
------------------------------------------------------------
Database Lifecycle
------------------------------------------------------------
*/

/**
 * @brief Create a new BlueCrab database at the specified path with the given name.
 *
 * @param path Filesystem path where the database will be created.
 * @param name Name of the database.
 * @return 0 on success, non-zero on failure.
 */
int fossil_db_bluecrab_create(
    const char *path,
    const char *name);

/**
 * @brief Open an existing BlueCrab database from the specified path.
 *
 * @param db Pointer to a fossil_bluecrab_db structure to initialize.
 * @param path Filesystem path to the database.
 * @return 0 on success, non-zero on failure.
 */
int fossil_db_bluecrab_open(
    fossil_bluecrab_db *db,
    const char *path);

/**
 * @brief Close an open BlueCrab database and release resources.
 *
 * @param db Pointer to the open fossil_bluecrab_db structure.
 * @return 0 on success, non-zero on failure.
 */
int fossil_db_bluecrab_close(
    fossil_bluecrab_db *db);

/**
 * @brief Delete a BlueCrab database at the specified path.
 *
 * @param path Filesystem path to the database to delete.
 * @return 0 on success, non-zero on failure.
 */
int fossil_db_bluecrab_delete(
    const char *path);

/*
------------------------------------------------------------
Entry CRUD Operations
------------------------------------------------------------
*/

/**
 * @brief Insert a new entry into the database.
 *
 * @param db Pointer to the open database.
 * @param id Unique identifier for the entry.
 * @param fson_data FSON-encoded data for the entry.
 * @return 0 on success, non-zero on failure.
 */
int fossil_db_bluecrab_insert(
    fossil_bluecrab_db *db,
    const char *id,
    const char *fson_data);

/**
 * @brief Retrieve an entry's FSON data by its ID.
 *
 * @param db Pointer to the open database.
 * @param id Unique identifier for the entry.
 * @param out_fson Output pointer to allocated FSON data (caller must free).
 * @return 0 on success, non-zero on failure.
 */
int fossil_db_bluecrab_get(
    fossil_bluecrab_db *db,
    const char *id,
    char **out_fson);

/**
 * @brief Update an existing entry's data.
 *
 * @param db Pointer to the open database.
 * @param id Unique identifier for the entry.
 * @param fson_data New FSON-encoded data for the entry.
 * @return 0 on success, non-zero on failure.
 */
int fossil_db_bluecrab_update(
    fossil_bluecrab_db *db,
    const char *id,
    const char *fson_data);

/**
 * @brief Remove (delete) an entry from the database by its ID.
 *
 * @param db Pointer to the open database.
 * @param id Unique identifier for the entry.
 * @return 0 on success, non-zero on failure.
 */
int fossil_db_bluecrab_remove(
    fossil_bluecrab_db *db,
    const char *id);

/*
------------------------------------------------------------
Sub Entry Operations
------------------------------------------------------------
*/

/**
 * @brief Insert a sub-entry under a parent entry.
 *
 * @param db Pointer to the open database.
 * @param parent_id ID of the parent entry.
 * @param sub_id Unique identifier for the sub-entry.
 * @param fson_data FSON-encoded data for the sub-entry.
 * @return 0 on success, non-zero on failure.
 */
int fossil_db_bluecrab_insert_sub(
    fossil_bluecrab_db *db,
    const char *parent_id,
    const char *sub_id,
    const char *fson_data);

/**
 * @brief Retrieve a sub-entry's FSON data by parent and sub-entry ID.
 *
 * @param db Pointer to the open database.
 * @param parent_id ID of the parent entry.
 * @param sub_id Unique identifier for the sub-entry.
 * @param out_fson Output pointer to allocated FSON data (caller must free).
 * @return 0 on success, non-zero on failure.
 */
int fossil_db_bluecrab_get_sub(
    fossil_bluecrab_db *db,
    const char *parent_id,
    const char *sub_id,
    char **out_fson);

/*
------------------------------------------------------------
Relationship Graph (DAG)
------------------------------------------------------------
*/

/**
 * @brief Create a relationship (edge) between two entries.
 *
 * @param db Pointer to the open database.
 * @param source_id ID of the source entry.
 * @param target_id ID of the target entry.
 * @param relation Type or name of the relationship.
 * @return 0 on success, non-zero on failure.
 */
int fossil_db_bluecrab_link(
    fossil_bluecrab_db *db,
    const char *source_id,
    const char *target_id,
    const char *relation);

/**
 * @brief Remove a relationship (edge) between two entries.
 *
 * @param db Pointer to the open database.
 * @param source_id ID of the source entry.
 * @param target_id ID of the target entry.
 * @return 0 on success, non-zero on failure.
 */
int fossil_db_bluecrab_unlink(
    fossil_bluecrab_db *db,
    const char *source_id,
    const char *target_id);

/**
 * @brief Retrieve all relationships for a given entry.
 *
 * @param db Pointer to the open database.
 * @param id Entry ID to query relationships for.
 * @param out_relations Output pointer to allocated array of relations (caller must free).
 * @param count Output pointer to number of relations found.
 * @return 0 on success, non-zero on failure.
 */
int fossil_db_bluecrab_get_relations(
    fossil_bluecrab_db *db,
    const char *id,
    fossil_bluecrab_relation **out_relations,
    size_t *count);

/*
------------------------------------------------------------
Search Operations
------------------------------------------------------------
*/

/**
 * @brief Perform an exact match search on a field and value.
 *
 * @param db Pointer to the open database.
 * @param field Field name to search.
 * @param value Value to match exactly.
 * @param results Output pointer to allocated array of search results (caller must free).
 * @param count Output pointer to number of results found.
 * @return 0 on success, non-zero on failure.
 */
int fossil_db_bluecrab_search_exact(
    fossil_bluecrab_db *db,
    const char *field,
    const char *value,
    fossil_bluecrab_search_result **results,
    size_t *count);

/**
 * @brief Perform a fuzzy search using a query string.
 *
 * @param db Pointer to the open database.
 * @param query Query string for fuzzy matching.
 * @param results Output pointer to allocated array of search results (caller must free).
 * @param count Output pointer to number of results found.
 * @return 0 on success, non-zero on failure.
 */
int fossil_db_bluecrab_search_fuzzy(
    fossil_bluecrab_db *db,
    const char *query,
    fossil_bluecrab_search_result **results,
    size_t *count);

/*
------------------------------------------------------------
AI Smart Search Helpers
------------------------------------------------------------
*/

/**
 * @brief Compute a similarity score between two strings.
 *
 * @param a First string.
 * @param b Second string.
 * @return Similarity score as a float (higher means more similar).
 */
float fossil_db_bluecrab_similarity(
    const char *a,
    const char *b);

/**
 * @brief Rank search results in-place based on their score.
 *
 * @param results Array of search results.
 * @param count Number of results in the array.
 * @return 0 on success, non-zero on failure.
 */
int fossil_db_bluecrab_rank_results(
    fossil_bluecrab_search_result *results,
    size_t count);

/*
------------------------------------------------------------
Hash + Integrity
------------------------------------------------------------
*/

/**
 * @brief Compute the hash for a given entry's data.
 *
 * @param data FSON-encoded data to hash.
 * @param out_hash Output buffer for the hash (must be at least FOSSIL_BLUECRAB_HASH_SIZE).
 * @return 0 on success, non-zero on failure.
 */
int fossil_db_bluecrab_hash_entry(
    const char *data,
    char out_hash[FOSSIL_BLUECRAB_HASH_SIZE]);

/**
 * @brief Verify the integrity of an entry by its ID.
 *
 * @param db Pointer to the open database.
 * @param id Unique identifier for the entry.
 * @return 0 if the entry is valid, non-zero if corrupted or invalid.
 */
int fossil_db_bluecrab_verify_entry(
    fossil_bluecrab_db *db,
    const char *id);

/*
------------------------------------------------------------
Git Hybrid Operations
------------------------------------------------------------
*/

/**
 * @brief Commit the current state of the database with a message.
 *
 * @param db Pointer to the open database.
 * @param message Commit message describing the changes.
 * @return 0 on success, non-zero on failure.
 */
int fossil_db_bluecrab_commit(
    fossil_bluecrab_db *db,
    const char *message);

/**
 * @brief Print the commit log/history of the database.
 *
 * @param db Pointer to the open database.
 * @return 0 on success, non-zero on failure.
 */
int fossil_db_bluecrab_log(
    fossil_bluecrab_db *db);

/**
 * @brief Checkout a specific version of the database.
 *
 * @param db Pointer to the open database.
 * @param version Version identifier to checkout.
 * @return 0 on success, non-zero on failure.
 */
int fossil_db_bluecrab_checkout(
    fossil_bluecrab_db *db,
    const char *version);

/*
------------------------------------------------------------
Meta Tree Operations
------------------------------------------------------------
*/

/**
 * @brief Load the database meta tree from disk.
 *
 * @param db Pointer to the open database.
 * @return 0 on success, non-zero on failure.
 */
int fossil_db_bluecrab_meta_load(
    fossil_bluecrab_db *db);

/**
 * @brief Save the database meta tree to disk.
 *
 * @param db Pointer to the open database.
 * @return 0 on success, non-zero on failure.
 */
int fossil_db_bluecrab_meta_save(
    fossil_bluecrab_db *db);

/**
 * @brief Rebuild the database meta tree from scratch.
 *
 * @param db Pointer to the open database.
 * @return 0 on success, non-zero on failure.
 */
int fossil_db_bluecrab_meta_rebuild(
    fossil_bluecrab_db *db);

/*
------------------------------------------------------------
Advanced Database Operations
------------------------------------------------------------
*/

/**
 * @brief Create a backup of the database at the specified path.
 *
 * @param db Pointer to the open database.
 * @param backup_path Filesystem path for the backup file.
 * @return 0 on success, non-zero on failure.
 */
int fossil_db_bluecrab_backup(
    fossil_bluecrab_db *db,
    const char *backup_path);

/**
 * @brief Restore the database from a backup file.
 *
 * @param db Pointer to the open database.
 * @param backup_path Filesystem path to the backup file.
 * @return 0 on success, non-zero on failure.
 */
int fossil_db_bluecrab_restore(
    fossil_bluecrab_db *db,
    const char *backup_path);

/**
 * @brief Compact the database to reclaim space and optimize storage.
 *
 * @param db Pointer to the open database.
 * @return 0 on success, non-zero on failure.
 */
int fossil_db_bluecrab_compact(
    fossil_bluecrab_db *db);

/**
 * @brief Verify the integrity of the entire database.
 *
 * @param db Pointer to the open database.
 * @return 0 if the database is valid, non-zero if corrupted.
 */
int fossil_db_bluecrab_verify(
    fossil_bluecrab_db *db);

#ifdef __cplusplus
}
#include <string>
#include <vector>

namespace fossil::db
{

    /*
    ------------------------------------------------------------
    BlueCrab C++ Wrapper
    ------------------------------------------------------------
    */

    class BlueCrab 
    {
    public:
        BlueCrab() : db_{} {}

        ~BlueCrab() {
            if (db_.opened) {
                fossil_db_bluecrab_close(&db_);
            }
        }

        // Create a new database (static, does not open)
        static int create(const std::string& path, const std::string& name) {
            return fossil_db_bluecrab_create(path.c_str(), name.c_str());
        }

        // Delete a database (static)
        static int remove(const std::string& path) {
            return fossil_db_bluecrab_delete(path.c_str());
        }

        // Open an existing database
        int open(const std::string& path) {
            int rc = fossil_db_bluecrab_open(&db_, path.c_str());
            return rc;
        }

        // Close the database
        int close() {
            int rc = fossil_db_bluecrab_close(&db_);
            return rc;
        }

        // Insert entry
        int insert(const std::string& id, const std::string& fson_data) {
            return fossil_db_bluecrab_insert(&db_, id.c_str(), fson_data.c_str());
        }

        // Get entry
        int get(const std::string& id, std::string& out_fson) {
            char* fson = nullptr;
            int rc = fossil_db_bluecrab_get(&db_, id.c_str(), &fson);
            if (rc == 0 && fson) {
                out_fson = fson;
                free(fson);
            }
            return rc;
        }

        // Update entry
        int update(const std::string& id, const std::string& fson_data) {
            return fossil_db_bluecrab_update(&db_, id.c_str(), fson_data.c_str());
        }

        // Remove entry
        int remove_entry(const std::string& id) {
            return fossil_db_bluecrab_remove(&db_, id.c_str());
        }

        // Insert sub-entry
        int insert_sub(const std::string& parent_id, const std::string& sub_id, const std::string& fson_data) {
            return fossil_db_bluecrab_insert_sub(&db_, parent_id.c_str(), sub_id.c_str(), fson_data.c_str());
        }

        // Get sub-entry
        int get_sub(const std::string& parent_id, const std::string& sub_id, std::string& out_fson) {
            char* fson = nullptr;
            int rc = fossil_db_bluecrab_get_sub(&db_, parent_id.c_str(), sub_id.c_str(), &fson);
            if (rc == 0 && fson) {
                out_fson = fson;
                free(fson);
            }
            return rc;
        }

        // Link entries
        int link(const std::string& source_id, const std::string& target_id, const std::string& relation) {
            return fossil_db_bluecrab_link(&db_, source_id.c_str(), target_id.c_str(), relation.c_str());
        }

        // Unlink entries
        int unlink(const std::string& source_id, const std::string& target_id) {
            return fossil_db_bluecrab_unlink(&db_, source_id.c_str(), target_id.c_str());
        }

        // Get relations
        int get_relations(const std::string& id, std::vector<fossil_bluecrab_relation>& out_relations) {
            fossil_bluecrab_relation* rels = nullptr;
            size_t count = 0;
            int rc = fossil_db_bluecrab_get_relations(&db_, id.c_str(), &rels, &count);
            if (rc == 0 && rels) {
                out_relations.assign(rels, rels + count);
                free(rels);
            }
            return rc;
        }

        // Exact search
        int search_exact(const std::string& field, const std::string& value, std::vector<fossil_bluecrab_search_result>& results) {
            fossil_bluecrab_search_result* arr = nullptr;
            size_t count = 0;
            int rc = fossil_db_bluecrab_search_exact(&db_, field.c_str(), value.c_str(), &arr, &count);
            if (rc == 0 && arr) {
                results.assign(arr, arr + count);
                free(arr);
            }
            return rc;
        }

        // Fuzzy search
        int search_fuzzy(const std::string& query, std::vector<fossil_bluecrab_search_result>& results) {
            fossil_bluecrab_search_result* arr = nullptr;
            size_t count = 0;
            int rc = fossil_db_bluecrab_search_fuzzy(&db_, query.c_str(), &arr, &count);
            if (rc == 0 && arr) {
                results.assign(arr, arr + count);
                free(arr);
            }
            return rc;
        }

        // Commit
        int commit(const std::string& message) {
            return fossil_db_bluecrab_commit(&db_, message.c_str());
        }

        // Log
        int log() {
            return fossil_db_bluecrab_log(&db_);
        }

        // Checkout
        int checkout(const std::string& version) {
            return fossil_db_bluecrab_checkout(&db_, version.c_str());
        }

        // Meta operations
        int meta_load() { return fossil_db_bluecrab_meta_load(&db_); }
        int meta_save() { return fossil_db_bluecrab_meta_save(&db_); }
        int meta_rebuild() { return fossil_db_bluecrab_meta_rebuild(&db_); }

        // Backup/restore
        int backup(const std::string& backup_path) {
            return fossil_db_bluecrab_backup(&db_, backup_path.c_str());
        }
        int restore(const std::string& backup_path) {
            return fossil_db_bluecrab_restore(&db_, backup_path.c_str());
        }

        // Compact
        int compact() { return fossil_db_bluecrab_compact(&db_); }

        // Verify
        int verify() { return fossil_db_bluecrab_verify(&db_); }

        // Access to last error
        std::string last_error() const { return db_.last_error; }

        // Access to db name
        std::string name() const { return db_.name; }

        // Is opened
        bool is_opened() const { return db_.opened; }

        // Entry count
        size_t entry_count() const { return db_.entry_count; }

        // Relation count
        size_t relation_count() const { return db_.relation_count; }

        // Last commit version/hash
        uint64_t last_commit_version() const { return db_.last_commit_version; }
        std::string last_commit_hash() const { return db_.last_commit_hash; }

        // Underlying C struct access (if needed)
        fossil_bluecrab_db* native_handle() { return &db_; }
        const fossil_bluecrab_db* native_handle() const { return &db_; }

    private:
        fossil_bluecrab_db db_;
    };

}

#endif

#endif /* FOSSIL_DB_FRAMEWORK_H */
