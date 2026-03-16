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
#include <stdexcept>

namespace fossil::db
{

    /*
    ------------------------------------------------------------
    BlueCrab C++ Wrapper
    ------------------------------------------------------------
    */

    class BlueCrab
    {
    private:
        fossil_bluecrab_db db{};
        bool opened = false;

    public:
        /*
        ------------------------------------------------------------
        Lifecycle
        ------------------------------------------------------------
        */

        BlueCrab() = default;

        BlueCrab(const std::string &path)
        {
            open(path);
        }

        ~BlueCrab()
        {
            if (opened)
                close();
        }

        static void create(const std::string &path, const std::string &name)
        {
            if (path.empty() || name.empty())
                throw std::invalid_argument("Database path and name must not be empty");
            if (fossil_db_bluecrab_create(path.c_str(), name.c_str()) != 0)
                throw std::runtime_error("bluecrab create failed: could not create database at path '" + path + "' with name '" + name + "'");
        }

        static void remove_db(const std::string &path)
        {
            if (fossil_db_bluecrab_delete(path.c_str()) != 0)
                throw std::runtime_error("bluecrab delete failed");
        }

        void open(const std::string &path)
        {
            if (fossil_db_bluecrab_open(&db, path.c_str()) != 0)
                throw std::runtime_error("bluecrab open failed");
            opened = true;
        }

        void close()
        {
            if (opened)
            {
                if (fossil_db_bluecrab_close(&db) != 0)
                    throw std::runtime_error("bluecrab close failed");
                opened = false;
            }
        }

        /*
        ------------------------------------------------------------
        CRUD
        ------------------------------------------------------------
        */

        void insert(const std::string &id, const std::string &fson)
        {
            if (fossil_db_bluecrab_insert(&db, id.c_str(), fson.c_str()) != 0)
                throw std::runtime_error("insert failed");
        }

        std::string get(const std::string &id)
        {
            char *result = nullptr;
            if (fossil_db_bluecrab_get(&db, id.c_str(), &result) != 0)
                throw std::runtime_error("get failed");
            std::string out = result;
            free(result);
            return out;
        }

        void update(const std::string &id, const std::string &fson)
        {
            if (fossil_db_bluecrab_update(&db, id.c_str(), fson.c_str()) != 0)
                throw std::runtime_error("update failed");
        }

        void remove(const std::string &id)
        {
            if (fossil_db_bluecrab_remove(&db, id.c_str()) != 0)
                throw std::runtime_error("remove failed");
        }

        /*
        ------------------------------------------------------------
        Sub Entries
        ------------------------------------------------------------
        */

        void insert_sub(const std::string &parent, const std::string &sub_id, const std::string &fson)
        {
            if (fossil_db_bluecrab_insert_sub(&db, parent.c_str(), sub_id.c_str(), fson.c_str()) != 0)
                throw std::runtime_error("insert_sub failed");
        }

        std::string get_sub(const std::string &parent, const std::string &sub_id)
        {
            char *result = nullptr;
            if (fossil_db_bluecrab_get_sub(&db, parent.c_str(), sub_id.c_str(), &result) != 0)
                throw std::runtime_error("get_sub failed");
            std::string out = result;
            free(result);
            return out;
        }

        /*
        ------------------------------------------------------------
        Relationships
        ------------------------------------------------------------
        */

        void link(const std::string &source, const std::string &target, const std::string &relation)
        {
            if (fossil_db_bluecrab_link(&db, source.c_str(), target.c_str(), relation.c_str()) != 0)
                throw std::runtime_error("link failed");
        }

        void unlink(const std::string &source, const std::string &target)
        {
            if (fossil_db_bluecrab_unlink(&db, source.c_str(), target.c_str()) != 0)
                throw std::runtime_error("unlink failed");
        }

        std::vector<fossil_bluecrab_relation> get_relations(const std::string &id)
        {
            fossil_bluecrab_relation *relations = nullptr;
            size_t count = 0;
            if (fossil_db_bluecrab_get_relations(&db, id.c_str(), &relations, &count) != 0)
                throw std::runtime_error("get_relations failed");
            std::vector<fossil_bluecrab_relation> vec(relations, relations + count);
            free(relations);
            return vec;
        }

        /*
        ------------------------------------------------------------
        Search
        ------------------------------------------------------------
        */

        std::vector<fossil_bluecrab_search_result> search_exact(const std::string &field, const std::string &value)
        {
            fossil_bluecrab_search_result *results = nullptr;
            size_t count = 0;
            if (fossil_db_bluecrab_search_exact(&db, field.c_str(), value.c_str(), &results, &count) != 0)
                throw std::runtime_error("search_exact failed");
            std::vector<fossil_bluecrab_search_result> vec(results, results + count);
            free(results);
            return vec;
        }

        std::vector<fossil_bluecrab_search_result> search_fuzzy(const std::string &query)
        {
            fossil_bluecrab_search_result *results = nullptr;
            size_t count = 0;
            if (fossil_db_bluecrab_search_fuzzy(&db, query.c_str(), &results, &count) != 0)
                throw std::runtime_error("search_fuzzy failed");
            std::vector<fossil_bluecrab_search_result> vec(results, results + count);
            free(results);
            return vec;
        }

        /*
        ------------------------------------------------------------
        AI Smart Search Helpers
        ------------------------------------------------------------
        */

        static float similarity(const std::string &a, const std::string &b)
        {
            return fossil_db_bluecrab_similarity(a.c_str(), b.c_str());
        }

        static void rank_results(std::vector<fossil_bluecrab_search_result> &results)
        {
            if (!results.empty())
            {
                if (fossil_db_bluecrab_rank_results(results.data(), results.size()) != 0)
                    throw std::runtime_error("rank_results failed");
            }
        }

        /*
        ------------------------------------------------------------
        Hash + Integrity
        ------------------------------------------------------------
        */

        std::string hash_entry(const std::string &data)
        {
            char hash[FOSSIL_BLUECRAB_HASH_SIZE] = {};
            if (fossil_db_bluecrab_hash_entry(data.c_str(), hash) != 0)
                throw std::runtime_error("hash_entry failed");
            return std::string(hash);
        }

        bool verify_entry(const std::string &id)
        {
            return fossil_db_bluecrab_verify_entry(&db, id.c_str()) == 0;
        }

        /*
        ------------------------------------------------------------
        Git Hybrid
        ------------------------------------------------------------
        */

        void commit(const std::string &message)
        {
            if (fossil_db_bluecrab_commit(&db, message.c_str()) != 0)
                throw std::runtime_error("commit failed");
        }

        void log()
        {
            if (fossil_db_bluecrab_log(&db) != 0)
                throw std::runtime_error("log failed");
        }

        void checkout(const std::string &version)
        {
            if (fossil_db_bluecrab_checkout(&db, version.c_str()) != 0)
                throw std::runtime_error("checkout failed");
        }

        /*
        ------------------------------------------------------------
        Meta Tree Operations
        ------------------------------------------------------------
        */

        void meta_load()
        {
            if (fossil_db_bluecrab_meta_load(&db) != 0)
                throw std::runtime_error("meta_load failed");
        }

        void meta_save()
        {
            if (fossil_db_bluecrab_meta_save(&db) != 0)
                throw std::runtime_error("meta_save failed");
        }

        void meta_rebuild()
        {
            if (fossil_db_bluecrab_meta_rebuild(&db) != 0)
                throw std::runtime_error("meta_rebuild failed");
        }

        /*
        ------------------------------------------------------------
        Advanced Database Operations
        ------------------------------------------------------------
        */

        void backup(const std::string &path)
        {
            if (fossil_db_bluecrab_backup(&db, path.c_str()) != 0)
                throw std::runtime_error("backup failed");
        }

        void restore(const std::string &path)
        {
            if (fossil_db_bluecrab_restore(&db, path.c_str()) != 0)
                throw std::runtime_error("restore failed");
        }

        void compact()
        {
            if (fossil_db_bluecrab_compact(&db) != 0)
                throw std::runtime_error("compact failed");
        }

        void verify()
        {
            if (fossil_db_bluecrab_verify(&db) != 0)
                throw std::runtime_error("verify failed");
        }
    };

}

#endif

#endif /* FOSSIL_DB_FRAMEWORK_H */
