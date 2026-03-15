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
#include <fossil/pizza/framework.h>

#include "fossil/crabdb/framework.h"

// * * * * * * * * * * * * * * * * * * * * * * * *
// * Fossil Logic Test Utilities
// * * * * * * * * * * * * * * * * * * * * * * * *
// Setup steps for things like test fixtures and
// mock objects are set here.
// * * * * * * * * * * * * * * * * * * * * * * * *

FOSSIL_SUITE(cpp_bluecrab_fixture);

FOSSIL_SETUP(cpp_bluecrab_fixture)
{
    // Setup the test fixture
}

FOSSIL_TEARDOWN(cpp_bluecrab_fixture)
{
    // Teardown the test fixture
}

// * * * * * * * * * * * * * * * * * * * * * * * *
// * Fossil Logic Test Blue CrabDB Database
// * * * * * * * * * * * * * * * * * * * * * * * *

#define TEST_DB_PATH "/tmp/bluecrab_testdb"
#define TEST_DB_NAME "TestDB"

// C++ wrapper test for BlueCrab class
FOSSIL_TEST(cpp_test_bluecrab_create_open_close_delete)
{
    FOSSIL_SANITY_SYS_DELETE_FILE(TEST_DB_PATH);

    // Static creation
    bool threw = false;
    try {
        fossil::db::BlueCrab::create(TEST_DB_PATH, TEST_DB_NAME);
    } catch (...) {
        threw = true;
    }
    ASSUME_NOT_TRUE(threw);

    // Open/close via C++ class
    fossil::db::BlueCrab db;
    threw = false;
    try {
        db.open(TEST_DB_PATH);
        db.close();
    } catch (...) {
        threw = true;
    }
    ASSUME_NOT_TRUE(threw);

    // Open via constructor
    threw = false;
    try {
        fossil::db::BlueCrab db2(TEST_DB_PATH);
        db2.close();
    } catch (...) {
        threw = true;
    }
    ASSUME_NOT_TRUE(threw);

    // Ensure file is closed before attempting to delete, and retry if needed
    int del_result = 0;
    try {
        del_result = fossil_db_bluecrab_delete(TEST_DB_PATH);
        if (del_result != 0) {
            FOSSIL_SANITY_SYS_SLEEP(1000); // 1 second
            del_result = fossil_db_bluecrab_delete(TEST_DB_PATH);
        }
    } catch (...) {
        del_result = -1;
    }
    ASSUME_ITS_TRUE(del_result == 0);
}

FOSSIL_TEST(cpp_test_bluecrab_crud_entry)
{
    FOSSIL_SANITY_SYS_DELETE_FILE(TEST_DB_PATH);

    // Create DB using static method
    bool threw = false;
    try {
        fossil::db::BlueCrab::create(TEST_DB_PATH, TEST_DB_NAME);
    } catch (...) {
        threw = true;
    }
    ASSUME_NOT_TRUE(threw);

    // Open DB using C++ wrapper
    fossil::db::BlueCrab db;
    threw = false;
    try {
        db.open(TEST_DB_PATH);
    } catch (...) {
        threw = true;
    }
    ASSUME_NOT_TRUE(threw);

    const std::string id = "entry1";
    const std::string fson = "object: { name: cstr:\"Alice\", age: i32:30 }";

    // Insert
    threw = false;
    try {
        db.insert(id, fson);
    } catch (...) {
        threw = true;
    }
    ASSUME_NOT_TRUE(threw);

    // Get
    std::string out;
    threw = false;
    try {
        out = db.get(id);
    } catch (...) {
        threw = true;
    }
    ASSUME_NOT_TRUE(threw);
    ASSUME_ITS_CSTR_CONTAINS(out.c_str(), "Alice");

    // Update
    const std::string fson2 = "object: { name: cstr:\"Bob\", age: i32:40 }";
    threw = false;
    try {
        db.update(id, fson2);
    } catch (...) {
        threw = true;
    }
    ASSUME_NOT_TRUE(threw);

    // Get updated
    threw = false;
    try {
        out = db.get(id);
    } catch (...) {
        threw = true;
    }
    ASSUME_NOT_TRUE(threw);
    ASSUME_ITS_CSTR_CONTAINS(out.c_str(), "Bob");

    // Remove
    threw = false;
    try {
        db.remove(id);
    } catch (...) {
        threw = true;
    }
    ASSUME_NOT_TRUE(threw);

    // Get after remove (should throw)
    bool get_failed = false;
    try {
        out = db.get(id);
    } catch (...) {
        get_failed = true;
    }
    ASSUME_ITS_TRUE(get_failed);

    try {
        db.close();
    } catch (...) {
        // ignore close errors
    }
    int del_result = 0;
    try {
        del_result = fossil_db_bluecrab_delete(TEST_DB_PATH);
    } catch (...) {
        del_result = -1;
    }
    ASSUME_ITS_TRUE(del_result == 0);
}

FOSSIL_TEST(cpp_test_bluecrab_subentry)
{
    FOSSIL_SANITY_SYS_DELETE_FILE(TEST_DB_PATH);
    fossil::db::BlueCrab::create(TEST_DB_PATH, TEST_DB_NAME);

    fossil::db::BlueCrab db;
    bool threw = false;
    try {
        db.open(TEST_DB_PATH);
    } catch (...) {
        threw = true;
    }
    ASSUME_NOT_TRUE(threw);

    const std::string parent = "parent";
    const std::string sub = "child";
    const std::string fson = "object: { value: i32:123 }";

    threw = false;
    try {
        db.insert_sub(parent, sub, fson);
    } catch (...) {
        threw = true;
    }
    ASSUME_NOT_TRUE(threw);

    std::string out;
    threw = false;
    try {
        out = db.get_sub(parent, sub);
    } catch (...) {
        threw = true;
    }
    ASSUME_NOT_TRUE(threw);
    ASSUME_ITS_CSTR_CONTAINS(out.c_str(), "123");

    db.close();
    ASSUME_ITS_TRUE(fossil_db_bluecrab_delete(TEST_DB_PATH) == 0);
}

FOSSIL_TEST(cpp_test_bluecrab_relations)
{
    FOSSIL_SANITY_SYS_DELETE_FILE(TEST_DB_PATH);
    fossil::db::BlueCrab::create(TEST_DB_PATH, TEST_DB_NAME);

    fossil::db::BlueCrab db;
    bool threw = false;
    try {
        db.open(TEST_DB_PATH);
    } catch (...) {
        threw = true;
    }
    ASSUME_NOT_TRUE(threw);

    try {
        db.insert("A", "object: { }");
        db.insert("B", "object: { }");
        db.insert("C", "object: { }");
        db.link("A", "B", "friend");
        db.link("A", "C", "colleague");
    } catch (...) {
        threw = true;
    }
    ASSUME_NOT_TRUE(threw);

    std::vector<fossil_bluecrab_relation> rels;
    threw = false;
    try {
        rels = db.get_relations("A");
    } catch (...) {
        threw = true;
    }
    ASSUME_NOT_TRUE(threw);
    ASSUME_ITS_MORE_OR_EQUAL_SIZE(rels.size(), 2);

    threw = false;
    try {
        db.unlink("A", "B");
        rels = db.get_relations("A");
    } catch (...) {
        threw = true;
    }
    ASSUME_NOT_TRUE(threw);
    ASSUME_ITS_MORE_OR_EQUAL_SIZE(rels.size(), 1);

    db.close();
    ASSUME_ITS_TRUE(fossil_db_bluecrab_delete(TEST_DB_PATH) == 0);
}

FOSSIL_TEST(cpp_test_bluecrab_search_exact_and_fuzzy)
{
    FOSSIL_SANITY_SYS_DELETE_FILE(TEST_DB_PATH);
    fossil::db::BlueCrab::create(TEST_DB_PATH, TEST_DB_NAME);

    fossil::db::BlueCrab db;
    bool threw = false;
    try {
        db.open(TEST_DB_PATH);
    } catch (...) {
        threw = true;
    }
    ASSUME_NOT_TRUE(threw);

    try {
        db.insert("e1", "object: { name: cstr:\"Alpha\", tag: cstr:\"red\" }");
        db.insert("e2", "object: { name: cstr:\"Beta\", tag: cstr:\"blue\" }");
        db.insert("e3", "object: { name: cstr:\"Gamma\", tag: cstr:\"red\" }");
    } catch (...) {
        threw = true;
    }
    ASSUME_NOT_TRUE(threw);

    std::vector<fossil_bluecrab_search_result> results;
    threw = false;
    try {
        results = db.search_exact("tag", "red");
    } catch (...) {
        threw = true;
    }
    ASSUME_NOT_TRUE(threw);
    ASSUME_ITS_EQUAL_SIZE(results.size(), 2);

    threw = false;
    try {
        results = db.search_fuzzy("Alpha");
    } catch (...) {
        threw = true;
    }
    ASSUME_NOT_TRUE(threw);
    ASSUME_ITS_EQUAL_SIZE(results.size(), 1);

    db.close();
    ASSUME_ITS_TRUE(fossil_db_bluecrab_delete(TEST_DB_PATH) == 0);
}

FOSSIL_TEST(cpp_test_bluecrab_hash_and_verify)
{
    FOSSIL_SANITY_SYS_DELETE_FILE(TEST_DB_PATH);
    fossil::db::BlueCrab::create(TEST_DB_PATH, TEST_DB_NAME);

    fossil::db::BlueCrab db;
    bool threw = false;
    try {
        db.open(TEST_DB_PATH);
    } catch (...) {
        threw = true;
    }
    ASSUME_NOT_TRUE(threw);

    const std::string id = "hashentry";
    const std::string fson = "object: { foo: cstr:\"bar\" }";
    threw = false;
    try {
        db.insert(id, fson);
    } catch (...) {
        threw = true;
    }
    ASSUME_NOT_TRUE(threw);

    std::string data;
    threw = false;
    try {
        data = db.get(id);
    } catch (...) {
        threw = true;
    }
    ASSUME_NOT_TRUE(threw);

    std::string hash;
    threw = false;
    try {
        hash = db.hash_entry(data);
    } catch (...) {
        threw = true;
    }
    ASSUME_NOT_TRUE(threw);

    bool verified = false;
    threw = false;
    try {
        verified = db.verify_entry(id);
    } catch (...) {
        threw = true;
    }
    ASSUME_NOT_TRUE(threw);
    ASSUME_ITS_TRUE(verified);

    db.close();
    ASSUME_ITS_TRUE(fossil_db_bluecrab_delete(TEST_DB_PATH) == 0);
}

FOSSIL_TEST(cpp_test_bluecrab_commit_log_checkout)
{
    FOSSIL_SANITY_SYS_DELETE_FILE(TEST_DB_PATH);
    fossil::db::BlueCrab::create(TEST_DB_PATH, TEST_DB_NAME);

    fossil::db::BlueCrab db;
    bool threw = false;
    try {
        db.open(TEST_DB_PATH);
        db.insert("c1", "object: { v: i32:1 }");
        db.commit("Initial commit");

        db.insert("c2", "object: { v: i32:2 }");
        db.commit("Second commit");

        db.log();

        // Try checkout (should not throw)
        db.checkout("1");
    } catch (...) {
        threw = true;
    }
    ASSUME_NOT_TRUE(threw);

    db.close();
    ASSUME_ITS_TRUE(fossil_db_bluecrab_delete(TEST_DB_PATH) == 0);
}

FOSSIL_TEST(cpp_test_bluecrab_meta_and_advanced)
{
    FOSSIL_SANITY_SYS_DELETE_FILE(TEST_DB_PATH);
    fossil::db::BlueCrab::create(TEST_DB_PATH, TEST_DB_NAME);

    fossil::db::BlueCrab db;
    bool threw = false;
    const std::string backup_path = "/tmp/bluecrab_testdb_backup";
    FOSSIL_SANITY_SYS_DELETE_FILE(backup_path.c_str());
    try {
        db.open(TEST_DB_PATH);

        db.meta_load();
        db.meta_save();
        db.meta_rebuild();

        db.backup(backup_path);
        db.restore(backup_path);

        db.compact();
        db.verify();
    } catch (...) {
        threw = true;
    }
    ASSUME_NOT_TRUE(threw);

    db.close();
    ASSUME_ITS_TRUE(fossil_db_bluecrab_delete(TEST_DB_PATH) == 0);
    FOSSIL_SANITY_SYS_DELETE_FILE(backup_path.c_str());
}

FOSSIL_TEST(cpp_test_bluecrab_multiple_entries_and_bulk_crud)
{
    FOSSIL_SANITY_SYS_DELETE_FILE(TEST_DB_PATH);
    fossil::db::BlueCrab::create(TEST_DB_PATH, TEST_DB_NAME);

    fossil::db::BlueCrab db;
    bool threw = false;
    try {
        db.open(TEST_DB_PATH);

        const std::string ids[] = {"id1", "id2", "id3", "id4"};
        const std::string fsons[] = {
            "object: { name: cstr:\"One\", value: i32:1 }",
            "object: { name: cstr:\"Two\", value: i32:2 }",
            "object: { name: cstr:\"Three\", value: i32:3 }",
            "object: { name: cstr:\"Four\", value: i32:4 }"
        };

        for (int i = 0; i < 4; ++i) {
            db.insert(ids[i], fsons[i]);
            std::string out = db.get(ids[i]);
            ASSUME_ITS_CSTR_CONTAINS(out.c_str(), "object");
        }

        // Remove all entries
        for (int i = 0; i < 4; ++i) {
            db.remove(ids[i]);
        }

        // Confirm removal (should throw)
        for (int i = 0; i < 4; ++i) {
            bool get_failed = false;
            try {
                db.get(ids[i]);
            } catch (...) {
                get_failed = true;
            }
            ASSUME_ITS_TRUE(get_failed);
        }
    } catch (...) {
        threw = true;
    }
    ASSUME_NOT_TRUE(threw);

    db.close();
    ASSUME_ITS_TRUE(fossil_db_bluecrab_delete(TEST_DB_PATH) == 0);
}

FOSSIL_TEST(cpp_test_bluecrab_subentry_update_and_remove)
{
    FOSSIL_SANITY_SYS_DELETE_FILE(TEST_DB_PATH);
    fossil::db::BlueCrab::create(TEST_DB_PATH, TEST_DB_NAME);

    fossil::db::BlueCrab db;
    bool threw = false;
    try {
        db.open(TEST_DB_PATH);

        const std::string parent = "parent2";
        const std::string sub = "child2";
        const std::string fson = "object: { value: i32:555 }";

        db.insert_sub(parent, sub, fson);
        std::string out = db.get_sub(parent, sub);
        ASSUME_ITS_CSTR_CONTAINS(out.c_str(), "555");

        // Update subentry
        const std::string fson2 = "object: { value: i32:777 }";
        std::string id = parent + "_" + sub;
        db.update(id, fson2);
        out = db.get_sub(parent, sub);
        ASSUME_ITS_CSTR_CONTAINS(out.c_str(), "777");

        // Remove subentry
        db.remove(id);

        // Confirm removal (should throw)
        bool get_failed = false;
        try {
            db.get_sub(parent, sub);
        } catch (...) {
            get_failed = true;
        }
        ASSUME_ITS_TRUE(get_failed);
    } catch (...) {
        threw = true;
    }
    ASSUME_NOT_TRUE(threw);

    db.close();
    ASSUME_ITS_TRUE(fossil_db_bluecrab_delete(TEST_DB_PATH) == 0);
}

FOSSIL_TEST(cpp_test_bluecrab_relation_metadata_and_types)
{
    FOSSIL_SANITY_SYS_DELETE_FILE(TEST_DB_PATH);
    fossil::db::BlueCrab::create(TEST_DB_PATH, TEST_DB_NAME);

    fossil::db::BlueCrab db;
    bool threw = false;
    try {
        db.open(TEST_DB_PATH);

        db.insert("X", "object: { }");
        db.insert("Y", "object: { }");

        // Link with different relation types
        db.link("X", "Y", "parent");
        db.link("Y", "X", "child");

        auto rels = db.get_relations("X");
        ASSUME_ITS_MORE_OR_EQUAL_SIZE(rels.size(), 2);
        bool found_parent = false, found_child = false;
        for (const auto &rel : rels) {
            if (strcmp(rel.relation_type, "parent") == 0) found_parent = true;
            if (strcmp(rel.relation_type, "child") == 0) found_child = true;
        }
        ASSUME_ITS_TRUE(found_parent && found_child);

        db.close();
    } catch (...) {
        threw = true;
    }
    ASSUME_NOT_TRUE(threw);
    ASSUME_ITS_TRUE(fossil_db_bluecrab_delete(TEST_DB_PATH) == 0);
}

FOSSIL_TEST(cpp_test_bluecrab_search_no_results_and_empty_db)
{
    FOSSIL_SANITY_SYS_DELETE_FILE(TEST_DB_PATH);
    fossil::db::BlueCrab::create(TEST_DB_PATH, TEST_DB_NAME);

    fossil::db::BlueCrab db;
    bool threw = false;
    try {
        db.open(TEST_DB_PATH);

        // Search in empty DB
        auto results = db.search_exact("field", "value");
        ASSUME_ITS_EQUAL_SIZE(results.size(), 0);

        // Insert unrelated entry
        db.insert("foo", "object: { name: cstr:\"bar\" }");
        results = db.search_exact("name", "baz");
        ASSUME_ITS_EQUAL_SIZE(results.size(), 0);

        // Fuzzy search with no match
        results = db.search_fuzzy("notfound");
        ASSUME_ITS_EQUAL_SIZE(results.size(), 0);

        db.close();
    } catch (...) {
        threw = true;
    }
    ASSUME_NOT_TRUE(threw);
    ASSUME_ITS_TRUE(fossil_db_bluecrab_delete(TEST_DB_PATH) == 0);
}

FOSSIL_TEST(cpp_test_bluecrab_commit_and_checkout_invalid_version)
{
    FOSSIL_SANITY_SYS_DELETE_FILE(TEST_DB_PATH);
    fossil::db::BlueCrab::create(TEST_DB_PATH, TEST_DB_NAME);

    fossil::db::BlueCrab db;
    bool threw = false;
    try {
        db.open(TEST_DB_PATH);

        db.insert("v1", "object: { v: i32:10 }");
        db.commit("Commit 1");

        // Try to checkout a non-existent version (should throw)
        bool checkout_failed = false;
        try {
            db.checkout("9999");
        } catch (...) {
            checkout_failed = true;
        }
        ASSUME_ITS_TRUE(checkout_failed);

        db.close();
    } catch (...) {
        threw = true;
    }
    ASSUME_NOT_TRUE(threw);
    ASSUME_ITS_TRUE(fossil_db_bluecrab_delete(TEST_DB_PATH) == 0);
}

FOSSIL_TEST(cpp_test_bluecrab_hash_consistency)
{
    FOSSIL_SANITY_SYS_DELETE_FILE(TEST_DB_PATH);
    fossil::db::BlueCrab::create(TEST_DB_PATH, TEST_DB_NAME);

    fossil::db::BlueCrab db;
    bool threw = false;
    try {
        db.open(TEST_DB_PATH);

        const std::string id = "hashcheck";
        const std::string fson = "object: { foo: cstr:\"baz\" }";
        db.insert(id, fson);

        std::string data1 = db.get(id);
        std::string hash1 = db.hash_entry(data1);

        // Hash again, should be the same
        std::string hash2 = db.hash_entry(data1);
        ASSUME_ITS_EQUAL_CSTR(hash1.c_str(), hash2.c_str());

        db.close();
    } catch (...) {
        threw = true;
    }
    ASSUME_NOT_TRUE(threw);
    ASSUME_ITS_TRUE(fossil_db_bluecrab_delete(TEST_DB_PATH) == 0);
}

FOSSIL_TEST(cpp_test_bluecrab_bulk_insert_and_fuzzy_rank)
{
    FOSSIL_SANITY_SYS_DELETE_FILE(TEST_DB_PATH);
    fossil::db::BlueCrab::create(TEST_DB_PATH, TEST_DB_NAME);

    fossil::db::BlueCrab db;
    bool threw = false;
    try {
        db.open(TEST_DB_PATH);

        // Bulk insert 100 entries with similar and distinct names
        for (int i = 0; i < 100; ++i) {
            char id[32], fson[128];
            snprintf(id, sizeof(id), "user_%02d", i);
            snprintf(fson, sizeof(fson), "object: { name: cstr:\"User%02d\", tag: cstr:\"%s\" }", i, (i % 2 == 0) ? "even" : "odd");
            db.insert(id, fson);
        }

        // Fuzzy search for "User1" (should match User10, User11, User12, etc.)
        auto results = db.search_fuzzy("User1");
        ASSUME_ITS_EQUAL_SIZE(results.size(), 10);

        // Rank results and check that the top result is "user_10" or "user_11"
        fossil::db::BlueCrab::rank_results(results);
        bool top_is_expected = (strcmp(results[0].id, "user_10") == 0 || strcmp(results[0].id, "user_11") == 0);
        ASSUME_ITS_TRUE(top_is_expected);

        db.close();
    } catch (...) {
        threw = true;
    }
    ASSUME_NOT_TRUE(threw);
    ASSUME_ITS_TRUE(fossil_db_bluecrab_delete(TEST_DB_PATH) == 0);
}

FOSSIL_TEST(cpp_test_bluecrab_dag_cycle_prevention)
{
    FOSSIL_SANITY_SYS_DELETE_FILE(TEST_DB_PATH);
    fossil::db::BlueCrab::create(TEST_DB_PATH, TEST_DB_NAME);

    fossil::db::BlueCrab db;
    bool threw = false;
    try {
        db.open(TEST_DB_PATH);

        // Insert three nodes
        db.insert("A", "object: { }");
        db.insert("B", "object: { }");
        db.insert("C", "object: { }");

        // Create a DAG: A -> B -> C
        db.link("A", "B", "edge");
        db.link("B", "C", "edge");

        // Attempt to create a cycle: C -> A (should throw)
        bool cycle_failed = false;
        try {
            db.link("C", "A", "edge");
        } catch (...) {
            cycle_failed = true;
        }
        ASSUME_ITS_TRUE(cycle_failed);

        db.close();
    } catch (...) {
        threw = true;
    }
    ASSUME_NOT_TRUE(threw);
    ASSUME_ITS_TRUE(fossil_db_bluecrab_delete(TEST_DB_PATH) == 0);
}

FOSSIL_TEST(cpp_test_bluecrab_subentry_bulk_and_removal)
{
    FOSSIL_SANITY_SYS_DELETE_FILE(TEST_DB_PATH);
    fossil::db::BlueCrab::create(TEST_DB_PATH, TEST_DB_NAME);

    fossil::db::BlueCrab db;
    bool threw = false;
    try {
        db.open(TEST_DB_PATH);

        // Insert parent entry
        const std::string parent = "parent_bulk";
        db.insert(parent, "object: { }");

        // Insert 20 sub-entries
        for (int i = 0; i < 20; ++i) {
            char subid[32], fson[64];
            snprintf(subid, sizeof(subid), "sub_%02d", i);
            snprintf(fson, sizeof(fson), "object: { value: i32:%d }", i * 10);
            db.insert_sub(parent, subid, fson);
        }

        // Remove all sub-entries
        for (int i = 0; i < 20; ++i) {
            char subid[32];
            snprintf(subid, sizeof(subid), "sub_%02d", i);
            std::string id = parent + "_" + subid;
            db.remove(id);
        }

        // Confirm removal
        for (int i = 0; i < 20; ++i) {
            char subid[32];
            snprintf(subid, sizeof(subid), "sub_%02d", i);
            bool get_failed = false;
            try {
                db.get_sub(parent, subid);
            } catch (...) {
                get_failed = true;
            }
            ASSUME_ITS_TRUE(get_failed);
        }

        db.close();
    } catch (...) {
        threw = true;
    }
    ASSUME_NOT_TRUE(threw);
    ASSUME_ITS_TRUE(fossil_db_bluecrab_delete(TEST_DB_PATH) == 0);
}

FOSSIL_TEST(cpp_test_bluecrab_backup_restore_integrity)
{
    FOSSIL_SANITY_SYS_DELETE_FILE(TEST_DB_PATH);
    fossil::db::BlueCrab::create(TEST_DB_PATH, TEST_DB_NAME);

    fossil::db::BlueCrab db;
    bool threw = false;
    const std::string backup_path = "/tmp/bluecrab_testdb_adv_backup";
    FOSSIL_SANITY_SYS_DELETE_FILE(backup_path.c_str());
    try {
        db.open(TEST_DB_PATH);

        // Insert entries
        db.insert("bk1", "object: { foo: cstr:\"bar\" }");
        db.insert("bk2", "object: { foo: cstr:\"baz\" }");

        // Backup
        db.backup(backup_path);

        // Remove one entry and verify it's gone
        db.remove("bk2");
        bool get_failed = false;
        try {
            db.get("bk2");
        } catch (...) {
            get_failed = true;
        }
        ASSUME_ITS_TRUE(get_failed);

        // Restore from backup and verify both entries exist
        db.restore(backup_path);
        std::string out = db.get("bk1");
        ASSUME_ITS_CSTR_CONTAINS(out.c_str(), "bar");
        out = db.get("bk2");
        ASSUME_ITS_CSTR_CONTAINS(out.c_str(), "baz");

        db.close();
    } catch (...) {
        threw = true;
    }
    ASSUME_NOT_TRUE(threw);
    ASSUME_ITS_TRUE(fossil_db_bluecrab_delete(TEST_DB_PATH) == 0);
    FOSSIL_SANITY_SYS_DELETE_FILE(backup_path.c_str());
}

// * * * * * * * * * * * * * * * * * * * * * * * *
// * Fossil Logic Test Pool
// * * * * * * * * * * * * * * * * * * * * * * * *
FOSSIL_TEST_GROUP(cpp_bluecrab_database_tests)
{
    FOSSIL_TEST_ADD(cpp_bluecrab_fixture, cpp_test_bluecrab_create_open_close_delete);
    FOSSIL_TEST_ADD(cpp_bluecrab_fixture, cpp_test_bluecrab_crud_entry);
    FOSSIL_TEST_ADD(cpp_bluecrab_fixture, cpp_test_bluecrab_subentry);
    FOSSIL_TEST_ADD(cpp_bluecrab_fixture, cpp_test_bluecrab_relations);
    FOSSIL_TEST_ADD(cpp_bluecrab_fixture, cpp_test_bluecrab_search_exact_and_fuzzy);
    FOSSIL_TEST_ADD(cpp_bluecrab_fixture, cpp_test_bluecrab_hash_and_verify);
    FOSSIL_TEST_ADD(cpp_bluecrab_fixture, cpp_test_bluecrab_commit_log_checkout);
    FOSSIL_TEST_ADD(cpp_bluecrab_fixture, cpp_test_bluecrab_meta_and_advanced);
    FOSSIL_TEST_ADD(cpp_bluecrab_fixture, cpp_test_bluecrab_multiple_entries_and_bulk_crud);
    FOSSIL_TEST_ADD(cpp_bluecrab_fixture, cpp_test_bluecrab_subentry_update_and_remove);
    FOSSIL_TEST_ADD(cpp_bluecrab_fixture, cpp_test_bluecrab_relation_metadata_and_types);
    FOSSIL_TEST_ADD(cpp_bluecrab_fixture, cpp_test_bluecrab_search_no_results_and_empty_db);
    FOSSIL_TEST_ADD(cpp_bluecrab_fixture, cpp_test_bluecrab_commit_and_checkout_invalid_version);
    FOSSIL_TEST_ADD(cpp_bluecrab_fixture, cpp_test_bluecrab_hash_consistency);
    FOSSIL_TEST_ADD(cpp_bluecrab_fixture, cpp_test_bluecrab_bulk_insert_and_fuzzy_rank);
    FOSSIL_TEST_ADD(cpp_bluecrab_fixture, cpp_test_bluecrab_dag_cycle_prevention);
    FOSSIL_TEST_ADD(cpp_bluecrab_fixture, cpp_test_bluecrab_subentry_bulk_and_removal);
    FOSSIL_TEST_ADD(cpp_bluecrab_fixture, cpp_test_bluecrab_backup_restore_integrity);

    FOSSIL_TEST_REGISTER(cpp_bluecrab_fixture);
} // end of tests
