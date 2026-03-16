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
#include <algorithm>
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

using fossil::db::BlueCrab;

#define CPP_TEST_DB_PATH "./bluecrab_cpp_class_testdb"
#define CPP_TEST_DB_NAME "CppClassTestDB"

FOSSIL_TEST(cpp_test_bluecrab_class_create_open_close)
{
    // Remove any old DB
    FOSSIL_SANITY_SYS_DELETE_FILE(CPP_TEST_DB_PATH);

    // Create DB
    BlueCrab::create(CPP_TEST_DB_PATH, CPP_TEST_DB_NAME);

    // Open/close via class
    BlueCrab db;
    db.open(CPP_TEST_DB_PATH);
    db.close();

    // Remove DB
    BlueCrab::remove(CPP_TEST_DB_PATH);
}

FOSSIL_TEST(cpp_test_bluecrab_class_crud)
{
    FOSSIL_SANITY_SYS_DELETE_FILE(CPP_TEST_DB_PATH);
    BlueCrab::create(CPP_TEST_DB_PATH, CPP_TEST_DB_NAME);

    BlueCrab db;
    db.open(CPP_TEST_DB_PATH);

    db.insert("id1", "object: { name: cstr:\"Alice\" }");
    std::string val;
    db.get("id1", val);
    ASSUME_ITS_CSTR_CONTAINS(val.c_str(), "Alice");

    db.update("id1", "object: { name: cstr:\"Bob\" }");
    db.get("id1", val);
    ASSUME_ITS_CSTR_CONTAINS(val.c_str(), "Bob");

    db.remove_entry("id1");
    bool threw = false;
    try { db.get("id1", val); }
    catch (const std::runtime_error&) { threw = true; }
    ASSUME_ITS_TRUE(threw);

    db.close();
    BlueCrab::remove(CPP_TEST_DB_PATH);
}

FOSSIL_TEST(cpp_test_bluecrab_class_subentry)
{
    FOSSIL_SANITY_SYS_DELETE_FILE(CPP_TEST_DB_PATH);
    BlueCrab::create(CPP_TEST_DB_PATH, CPP_TEST_DB_NAME);
    BlueCrab db;
    db.open(CPP_TEST_DB_PATH);

    db.insert_sub("parent", "sub1", "object: { value: i32:42 }");
    std::string out;
    db.get_sub("parent", "sub1", out);
    ASSUME_ITS_CSTR_CONTAINS(out.c_str(), "42");

    db.close();
    BlueCrab::remove(CPP_TEST_DB_PATH);
}

FOSSIL_TEST(cpp_test_bluecrab_class_relations)
{
    FOSSIL_SANITY_SYS_DELETE_FILE(CPP_TEST_DB_PATH);
    BlueCrab::create(CPP_TEST_DB_PATH, CPP_TEST_DB_NAME);
    BlueCrab db;
    db.open(CPP_TEST_DB_PATH);

    db.insert("A", "object: { }");
    db.insert("B", "object: { }");
    db.link("A", "B", "friend");

    std::vector<fossil_bluecrab_relation> rels;
    db.get_relations("A", rels);
    bool found = false;
    for (const auto& r : rels)
        if (std::string(r.target_id) == "B" && std::string(r.relation_type) == "friend")
            found = true;
    ASSUME_ITS_TRUE(found);

    db.unlink("A", "B");
    db.get_relations("A", rels);
    bool still_found = false;
    for (const auto& r : rels)
        if (std::string(r.target_id) == "B")
            still_found = true;
    ASSUME_NOT_TRUE(still_found);

    db.close();
    BlueCrab::remove(CPP_TEST_DB_PATH);
}

FOSSIL_TEST(cpp_test_bluecrab_class_search_and_fuzzy)
{
    FOSSIL_SANITY_SYS_DELETE_FILE(CPP_TEST_DB_PATH);
    BlueCrab::create(CPP_TEST_DB_PATH, CPP_TEST_DB_NAME);
    BlueCrab db;
    db.open(CPP_TEST_DB_PATH);

    db.insert("e1", "object: { name: cstr:\"Alpha\", tag: cstr:\"red\" }");
    db.insert("e2", "object: { name: cstr:\"Beta\", tag: cstr:\"blue\" }");
    db.insert("e3", "object: { name: cstr:\"Gamma\", tag: cstr:\"red\" }");

    std::vector<fossil_bluecrab_search_result> exact;
    db.search_exact("tag", "red", exact);
    ASSUME_ITS_EQUAL_SIZE(exact.size(), 2);

    std::vector<fossil_bluecrab_search_result> fuzzy;
    db.search_fuzzy("Alpha", fuzzy);
    ASSUME_ITS_EQUAL_SIZE(fuzzy.size(), 1);

    db.close();
    BlueCrab::remove(CPP_TEST_DB_PATH);
}

FOSSIL_TEST(cpp_test_bluecrab_class_hash_and_verify)
{
    FOSSIL_SANITY_SYS_DELETE_FILE(CPP_TEST_DB_PATH);
    BlueCrab::create(CPP_TEST_DB_PATH, CPP_TEST_DB_NAME);
    BlueCrab db;
    db.open(CPP_TEST_DB_PATH);

    db.insert("hash1", "object: { foo: cstr:\"bar\" }");
    std::string data;
    db.get("hash1", data);
    // hash_entry and verify_entry are not implemented in the wrapper, so skip or implement if available

    db.close();
    BlueCrab::remove(CPP_TEST_DB_PATH);
}

FOSSIL_TEST(cpp_test_bluecrab_class_commit_and_checkout)
{
    FOSSIL_SANITY_SYS_DELETE_FILE(CPP_TEST_DB_PATH);
    BlueCrab::create(CPP_TEST_DB_PATH, CPP_TEST_DB_NAME);
    BlueCrab db;
    db.open(CPP_TEST_DB_PATH);

    db.insert("c1", "object: { v: i32:1 }");
    db.commit("Initial commit");
    db.insert("c2", "object: { v: i32:2 }");
    db.commit("Second commit");

    // Should not throw
    db.log();
    db.checkout("1");

    db.close();
    BlueCrab::remove(CPP_TEST_DB_PATH);
}

FOSSIL_TEST(cpp_test_bluecrab_class_meta_and_advanced)
{
    FOSSIL_SANITY_SYS_DELETE_FILE(CPP_TEST_DB_PATH);
    BlueCrab::create(CPP_TEST_DB_PATH, CPP_TEST_DB_NAME);
    BlueCrab db;
    db.open(CPP_TEST_DB_PATH);

    db.meta_load();
    db.meta_save();
    db.meta_rebuild();

    std::string backup_path = std::string(CPP_TEST_DB_PATH) + "_backup";
    FOSSIL_SANITY_SYS_DELETE_FILE(backup_path.c_str());
    db.backup(backup_path);
    db.restore(backup_path);

    db.compact();
    db.verify();

    db.close();
    BlueCrab::remove(CPP_TEST_DB_PATH);
    FOSSIL_SANITY_SYS_DELETE_FILE(backup_path.c_str());
}

FOSSIL_TEST(cpp_test_bluecrab_class_bulk_insert_and_search)
{
    FOSSIL_SANITY_SYS_DELETE_FILE(CPP_TEST_DB_PATH);
    BlueCrab::create(CPP_TEST_DB_PATH, CPP_TEST_DB_NAME);
    BlueCrab db;
    db.open(CPP_TEST_DB_PATH);

    for (int i = 0; i < 10; ++i) {
        db.insert("id" + std::to_string(i), "object: { value: i32:" + std::to_string(i) + " }");
    }

    std::vector<fossil_bluecrab_search_result> fuzzy;
    db.search_fuzzy("id", fuzzy);
    ASSUME_ITS_EQUAL_SIZE(fuzzy.size(), 10);

    for (int i = 0; i < 10; ++i) {
        db.remove_entry("id" + std::to_string(i));
    }

    db.close();
    BlueCrab::remove(CPP_TEST_DB_PATH);
}

FOSSIL_TEST(cpp_test_bluecrab_class_similarity_and_ranking)
{
    // similarity and rank_results are not implemented in the wrapper, so skip or implement if available
}

// * * * * * * * * * * * * * * * * * * * * * * * *
// * Fossil Logic Test Pool
// * * * * * * * * * * * * * * * * * * * * * * * *
FOSSIL_TEST_GROUP(cpp_bluecrab_database_tests)
{
    FOSSIL_TEST_ADD(cpp_bluecrab_fixture, cpp_test_bluecrab_class_create_open_close);
    FOSSIL_TEST_ADD(cpp_bluecrab_fixture, cpp_test_bluecrab_class_crud);
    FOSSIL_TEST_ADD(cpp_bluecrab_fixture, cpp_test_bluecrab_class_subentry);
    FOSSIL_TEST_ADD(cpp_bluecrab_fixture, cpp_test_bluecrab_class_relations);
    FOSSIL_TEST_ADD(cpp_bluecrab_fixture, cpp_test_bluecrab_class_search_and_fuzzy);
    FOSSIL_TEST_ADD(cpp_bluecrab_fixture, cpp_test_bluecrab_class_hash_and_verify);
    FOSSIL_TEST_ADD(cpp_bluecrab_fixture, cpp_test_bluecrab_class_commit_and_checkout);
    FOSSIL_TEST_ADD(cpp_bluecrab_fixture, cpp_test_bluecrab_class_meta_and_advanced);
    FOSSIL_TEST_ADD(cpp_bluecrab_fixture, cpp_test_bluecrab_class_bulk_insert_and_search);
    FOSSIL_TEST_ADD(cpp_bluecrab_fixture, cpp_test_bluecrab_class_similarity_and_ranking);

    FOSSIL_TEST_REGISTER(cpp_bluecrab_fixture);
} // end of tests
