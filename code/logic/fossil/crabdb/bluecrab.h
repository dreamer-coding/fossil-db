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
#ifndef FOSSIL_CRABDB_NOSHELL_H
#define FOSSIL_CRABDB_NOSHELL_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
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

typedef struct fossil_bluecrab_db {
    char name[128];
    char root_path[FOSSIL_BLUECRAB_PATH];
    char meta_path[FOSSIL_BLUECRAB_PATH];
    bool opened;
} fossil_bluecrab_db;

typedef struct fossil_bluecrab_entry {
    char id[FOSSIL_BLUECRAB_MAX_ID];
    char parent_id[FOSSIL_BLUECRAB_MAX_ID];
    char hash[FOSSIL_BLUECRAB_HASH_SIZE];
    uint64_t version;
} fossil_bluecrab_entry;

typedef struct fossil_bluecrab_relation {
    char source_id[FOSSIL_BLUECRAB_MAX_ID];
    char target_id[FOSSIL_BLUECRAB_MAX_ID];
    char relation_type[64];
} fossil_bluecrab_relation;

typedef struct fossil_bluecrab_search_result {
    char id[FOSSIL_BLUECRAB_MAX_ID];
    float score;
} fossil_bluecrab_search_result;


/*
------------------------------------------------------------
Database Lifecycle
------------------------------------------------------------
*/

int fossil_db_bluecrab_create(
    const char *path,
    const char *name
);

int fossil_db_bluecrab_open(
    fossil_bluecrab_db *db,
    const char *path
);

int fossil_db_bluecrab_close(
    fossil_bluecrab_db *db
);

int fossil_db_bluecrab_delete(
    const char *path
);


/*
------------------------------------------------------------
Entry CRUD Operations
------------------------------------------------------------
*/

int fossil_db_bluecrab_insert(
    fossil_bluecrab_db *db,
    const char *id,
    const char *fson_data
);

int fossil_db_bluecrab_get(
    fossil_bluecrab_db *db,
    const char *id,
    char **out_fson
);

int fossil_db_bluecrab_update(
    fossil_bluecrab_db *db,
    const char *id,
    const char *fson_data
);

int fossil_db_bluecrab_remove(
    fossil_bluecrab_db *db,
    const char *id
);


/*
------------------------------------------------------------
Sub Entry Operations
------------------------------------------------------------
*/

int fossil_db_bluecrab_insert_sub(
    fossil_bluecrab_db *db,
    const char *parent_id,
    const char *sub_id,
    const char *fson_data
);

int fossil_db_bluecrab_get_sub(
    fossil_bluecrab_db *db,
    const char *parent_id,
    const char *sub_id,
    char **out_fson
);


/*
------------------------------------------------------------
Relationship Graph (DAG)
------------------------------------------------------------
*/

int fossil_db_bluecrab_link(
    fossil_bluecrab_db *db,
    const char *source_id,
    const char *target_id,
    const char *relation
);

int fossil_db_bluecrab_unlink(
    fossil_bluecrab_db *db,
    const char *source_id,
    const char *target_id
);

int fossil_db_bluecrab_get_relations(
    fossil_bluecrab_db *db,
    const char *id,
    fossil_bluecrab_relation **out_relations,
    size_t *count
);


/*
------------------------------------------------------------
Search Operations
------------------------------------------------------------
*/

int fossil_db_bluecrab_search_exact(
    fossil_bluecrab_db *db,
    const char *field,
    const char *value,
    fossil_bluecrab_search_result **results,
    size_t *count
);

int fossil_db_bluecrab_search_fuzzy(
    fossil_bluecrab_db *db,
    const char *query,
    fossil_bluecrab_search_result **results,
    size_t *count
);


/*
------------------------------------------------------------
AI Smart Search Helpers
------------------------------------------------------------
*/

float fossil_db_bluecrab_similarity(
    const char *a,
    const char *b
);

int fossil_db_bluecrab_rank_results(
    fossil_bluecrab_search_result *results,
    size_t count
);


/*
------------------------------------------------------------
Hash + Integrity
------------------------------------------------------------
*/

int fossil_db_bluecrab_hash_entry(
    const char *data,
    char out_hash[FOSSIL_BLUECRAB_HASH_SIZE]
);

int fossil_db_bluecrab_verify_entry(
    fossil_bluecrab_db *db,
    const char *id
);


/*
------------------------------------------------------------
Git Hybrid Operations
------------------------------------------------------------
*/

int fossil_db_bluecrab_commit(
    fossil_bluecrab_db *db,
    const char *message
);

int fossil_db_bluecrab_log(
    fossil_bluecrab_db *db
);

int fossil_db_bluecrab_checkout(
    fossil_bluecrab_db *db,
    const char *version
);


/*
------------------------------------------------------------
Meta Tree Operations
------------------------------------------------------------
*/

int fossil_db_bluecrab_meta_load(
    fossil_bluecrab_db *db
);

int fossil_db_bluecrab_meta_save(
    fossil_bluecrab_db *db
);

int fossil_db_bluecrab_meta_rebuild(
    fossil_bluecrab_db *db
);


/*
------------------------------------------------------------
Advanced Database Operations
------------------------------------------------------------
*/

int fossil_db_bluecrab_backup(
    fossil_bluecrab_db *db,
    const char *backup_path
);

int fossil_db_bluecrab_restore(
    fossil_bluecrab_db *db,
    const char *backup_path
);

int fossil_db_bluecrab_compact(
    fossil_bluecrab_db *db
);

int fossil_db_bluecrab_verify(
    fossil_bluecrab_db *db
);

#ifdef __cplusplus
}
#include <string>
#include <vector>
#include <stdexcept>

namespace fossil::db {

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

    BlueCrab(const std::string& path)
    {
        open(path);
    }

    ~BlueCrab()
    {
        if (opened)
            close();
    }


    /*
    ------------------------------------------------------------
    Static Database Creation
    ------------------------------------------------------------
    */

    static void create(
        const std::string& path,
        const std::string& name
    )
    {
        if (fossil_db_bluecrab_create(path.c_str(), name.c_str()) != 0)
            throw std::runtime_error("bluecrab create failed");
    }


    /*
    ------------------------------------------------------------
    Open / Close
    ------------------------------------------------------------
    */

    void open(const std::string& path)
    {
        if (fossil_db_bluecrab_open(&db, path.c_str()) != 0)
            throw std::runtime_error("bluecrab open failed");

        opened = true;
    }


    void close()
    {
        fossil_db_bluecrab_close(&db);
        opened = false;
    }


    /*
    ------------------------------------------------------------
    CRUD
    ------------------------------------------------------------
    */

    void insert(
        const std::string& id,
        const std::string& fson
    )
    {
        if (fossil_db_bluecrab_insert(&db, id.c_str(), fson.c_str()) != 0)
            throw std::runtime_error("insert failed");
    }


    std::string get(const std::string& id)
    {
        char* result = nullptr;

        if (fossil_db_bluecrab_get(&db, id.c_str(), &result) != 0)
            throw std::runtime_error("get failed");

        std::string out = result;
        free(result);

        return out;
    }


    void update(
        const std::string& id,
        const std::string& fson
    )
    {
        if (fossil_db_bluecrab_update(&db, id.c_str(), fson.c_str()) != 0)
            throw std::runtime_error("update failed");
    }


    void remove(const std::string& id)
    {
        if (fossil_db_bluecrab_remove(&db, id.c_str()) != 0)
            throw std::runtime_error("remove failed");
    }


    /*
    ------------------------------------------------------------
    Sub Entries
    ------------------------------------------------------------
    */

    void insert_sub(
        const std::string& parent,
        const std::string& sub_id,
        const std::string& fson
    )
    {
        if (fossil_db_bluecrab_insert_sub(
                &db,
                parent.c_str(),
                sub_id.c_str(),
                fson.c_str()) != 0)
        {
            throw std::runtime_error("insert_sub failed");
        }
    }


    std::string get_sub(
        const std::string& parent,
        const std::string& sub_id
    )
    {
        char* result = nullptr;

        if (fossil_db_bluecrab_get_sub(
                &db,
                parent.c_str(),
                sub_id.c_str(),
                &result) != 0)
        {
            throw std::runtime_error("get_sub failed");
        }

        std::string out = result;
        free(result);

        return out;
    }


    /*
    ------------------------------------------------------------
    Relationships
    ------------------------------------------------------------
    */

    void link(
        const std::string& source,
        const std::string& target,
        const std::string& relation
    )
    {
        if (fossil_db_bluecrab_link(
                &db,
                source.c_str(),
                target.c_str(),
                relation.c_str()) != 0)
        {
            throw std::runtime_error("link failed");
        }
    }


    void unlink(
        const std::string& source,
        const std::string& target
    )
    {
        if (fossil_db_bluecrab_unlink(
                &db,
                source.c_str(),
                target.c_str()) != 0)
        {
            throw std::runtime_error("unlink failed");
        }
    }


    /*
    ------------------------------------------------------------
    Search
    ------------------------------------------------------------
    */

    std::vector<fossil_bluecrab_search_result> search_exact(
        const std::string& field,
        const std::string& value
    )
    {
        fossil_bluecrab_search_result* results = nullptr;
        size_t count = 0;

        fossil_db_bluecrab_search_exact(
            &db,
            field.c_str(),
            value.c_str(),
            &results,
            &count
        );

        std::vector<fossil_bluecrab_search_result> vec;

        for (size_t i = 0; i < count; ++i)
            vec.push_back(results[i]);

        free(results);

        return vec;
    }


    std::vector<fossil_bluecrab_search_result> search_fuzzy(
        const std::string& query
    )
    {
        fossil_bluecrab_search_result* results = nullptr;
        size_t count = 0;

        fossil_db_bluecrab_search_fuzzy(
            &db,
            query.c_str(),
            &results,
            &count
        );

        std::vector<fossil_bluecrab_search_result> vec;

        for (size_t i = 0; i < count; ++i)
            vec.push_back(results[i]);

        free(results);

        return vec;
    }


    /*
    ------------------------------------------------------------
    Hash
    ------------------------------------------------------------
    */

    std::string hash_entry(const std::string& data)
    {
        char hash[FOSSIL_BLUECRAB_HASH_SIZE];

        fossil_db_bluecrab_hash_entry(data.c_str(), hash);

        return std::string(hash);
    }


    bool verify_entry(const std::string& id)
    {
        return fossil_db_bluecrab_verify_entry(&db, id.c_str()) == 0;
    }


    /*
    ------------------------------------------------------------
    Git Hybrid
    ------------------------------------------------------------
    */

    void commit(const std::string& message)
    {
        if (fossil_db_bluecrab_commit(&db, message.c_str()) != 0)
            throw std::runtime_error("commit failed");
    }


    void log()
    {
        fossil_db_bluecrab_log(&db);
    }


    void checkout(const std::string& version)
    {
        fossil_db_bluecrab_checkout(&db, version.c_str());
    }


    /*
    ------------------------------------------------------------
    Maintenance
    ------------------------------------------------------------
    */

    void compact()
    {
        fossil_db_bluecrab_compact(&db);
    }


    void verify()
    {
        fossil_db_bluecrab_verify(&db);
    }


    void backup(const std::string& path)
    {
        fossil_db_bluecrab_backup(&db, path.c_str());
    }


    void restore(const std::string& path)
    {
        fossil_db_bluecrab_restore(&db, path.c_str());
    }
};

}

#endif

#endif /* FOSSIL_DB_FRAMEWORK_H */
