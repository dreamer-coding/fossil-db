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

FOSSIL_TEST(cpp_test_bluecrab_wrapper_create_open_close)
{
    using fossil::db::BlueCrab;
    const std::string path = "/tmp/bluecrab_wrapper_testdb";
    const std::string name = "CppWrapperDB";
    FOSSIL_SANITY_SYS_DELETE_FILE(path.c_str());

    // Static create
    BlueCrab::create(path, name);

    // Open/close via constructor and destructor
    {
        BlueCrab db(path);
        // Should be open, insert and get
        db.insert("id", "object: { foo: cstr:\"bar\" }");
        std::string val = db.get("id");
        ASSUME_ITS_CSTR_CONTAINS(val.c_str(), "bar");
    }

    // Open/close via open()/close()
    BlueCrab db2;
    db2.open(path);
    db2.insert("id2", "object: { foo: cstr:\"baz\" }");
    std::string val2 = db2.get("id2");
    ASSUME_ITS_CSTR_CONTAINS(val2.c_str(), "baz");
    db2.close();

    FOSSIL_SANITY_SYS_DELETE_FILE(path.c_str());
}

FOSSIL_TEST(cpp_test_bluecrab_wrapper_crud)
{
    using fossil::db::BlueCrab;
    const std::string path = "/tmp/bluecrab_wrapper_crud";
    const std::string name = "CrudDB";
    FOSSIL_SANITY_SYS_DELETE_FILE(path.c_str());
    BlueCrab::create(path, name);
    BlueCrab db(path);

    db.insert("e1", "object: { name: cstr:\"Alice\" }");
    std::string v = db.get("e1");
    ASSUME_ITS_CSTR_CONTAINS(v.c_str(), "Alice");

    db.update("e1", "object: { name: cstr:\"Bob\" }");
    v = db.get("e1");
    ASSUME_ITS_CSTR_CONTAINS(v.c_str(), "Bob");

    db.remove("e1");
    bool threw = false;
    try { db.get("e1"); }
    catch (const std::runtime_error&) { threw = true; }
    ASSUME_ITS_TRUE(threw);

    db.close();
    FOSSIL_SANITY_SYS_DELETE_FILE(path.c_str());
}

FOSSIL_TEST(cpp_test_bluecrab_wrapper_subentry)
{
    using fossil::db::BlueCrab;
    const std::string path = "/tmp/bluecrab_wrapper_sub";
    const std::string name = "SubDB";
    FOSSIL_SANITY_SYS_DELETE_FILE(path.c_str());
    BlueCrab::create(path, name);
    BlueCrab db(path);

    db.insert("parent", "object: { }");
    db.insert_sub("parent", "sub1", "object: { value: i32:42 }");
    std::string subval = db.get_sub("parent", "sub1");
    ASSUME_ITS_CSTR_CONTAINS(subval.c_str(), "42");

    db.close();
    FOSSIL_SANITY_SYS_DELETE_FILE(path.c_str());
}

FOSSIL_TEST(cpp_test_bluecrab_wrapper_link_unlink)
{
    using fossil::db::BlueCrab;
    const std::string path = "/tmp/bluecrab_wrapper_link";
    const std::string name = "LinkDB";
    FOSSIL_SANITY_SYS_DELETE_FILE(path.c_str());
    BlueCrab::create(path, name);
    BlueCrab db(path);

    db.insert("A", "object: { }");
    db.insert("B", "object: { }");
    db.link("A", "B", "friend");
    // Unlink should not throw
    db.unlink("A", "B");

    db.close();
    FOSSIL_SANITY_SYS_DELETE_FILE(path.c_str());
}

FOSSIL_TEST(cpp_test_bluecrab_wrapper_search)
{
    using fossil::db::BlueCrab;
    const std::string path = "/tmp/bluecrab_wrapper_search";
    const std::string name = "SearchDB";
    FOSSIL_SANITY_SYS_DELETE_FILE(path.c_str());
    BlueCrab::create(path, name);
    BlueCrab db(path);

    db.insert("e1", "object: { name: cstr:\"Alpha\", tag: cstr:\"red\" }");
    db.insert("e2", "object: { name: cstr:\"Beta\", tag: cstr:\"blue\" }");
    db.insert("e3", "object: { name: cstr:\"Gamma\", tag: cstr:\"red\" }");

    auto exact = db.search_exact("tag", "red");
    ASSUME_ITS_MORE_OR_EQUAL_SIZE(exact.size(), 2);

    auto fuzzy = db.search_fuzzy("Alpha");
    ASSUME_ITS_EQUAL_SIZE(fuzzy.size(), 1);

    db.close();
    FOSSIL_SANITY_SYS_DELETE_FILE(path.c_str());
}

FOSSIL_TEST(cpp_test_bluecrab_wrapper_hash_and_verify)
{
    using fossil::db::BlueCrab;
    const std::string path = "/tmp/bluecrab_wrapper_hash";
    const std::string name = "HashDB";
    FOSSIL_SANITY_SYS_DELETE_FILE(path.c_str());
    BlueCrab::create(path, name);
    BlueCrab db(path);

    db.insert("id", "object: { foo: cstr:\"bar\" }");
    std::string data = db.get("id");
    std::string hash = db.hash_entry(data);
    ASSUME_ITS_TRUE(!hash.empty());
    ASSUME_ITS_TRUE(db.verify_entry("id"));

    db.close();
    FOSSIL_SANITY_SYS_DELETE_FILE(path.c_str());
}

FOSSIL_TEST(cpp_test_bluecrab_wrapper_commit_log_checkout)
{
    using fossil::db::BlueCrab;
    const std::string path = "/tmp/bluecrab_wrapper_commit";
    const std::string name = "CommitDB";
    FOSSIL_SANITY_SYS_DELETE_FILE(path.c_str());
    BlueCrab::create(path, name);
    BlueCrab db(path);

    db.insert("c1", "object: { v: i32:1 }");
    db.commit("Initial commit");
    db.insert("c2", "object: { v: i32:2 }");
    db.commit("Second commit");
    db.log();
    db.checkout("1"); // Should not throw

    db.close();
    FOSSIL_SANITY_SYS_DELETE_FILE(path.c_str());
}

FOSSIL_TEST(cpp_test_bluecrab_wrapper_compact_verify_backup_restore)
{
    using fossil::db::BlueCrab;
    const std::string path = "/tmp/bluecrab_wrapper_maint";
    const std::string name = "MaintDB";
    const std::string backup = "/tmp/bluecrab_wrapper_maint_backup";
    FOSSIL_SANITY_SYS_DELETE_FILE(path.c_str());
    FOSSIL_SANITY_SYS_DELETE_FILE(backup.c_str());
    BlueCrab::create(path, name);
    BlueCrab db(path);

    db.insert("bk1", "object: { foo: cstr:\"bar\" }");
    db.insert("bk2", "object: { foo: cstr:\"baz\" }");
    db.compact();
    db.verify();
    db.backup(backup);

    db.remove("bk2");
    bool threw = false;
    try { db.get("bk2"); }
    catch (const std::runtime_error&) { threw = true; }
    ASSUME_ITS_TRUE(threw);

    db.restore(backup);
    std::string v1 = db.get("bk1");
    std::string v2 = db.get("bk2");
    ASSUME_ITS_CSTR_CONTAINS(v1.c_str(), "bar");
    ASSUME_ITS_CSTR_CONTAINS(v2.c_str(), "baz");

    db.close();
    FOSSIL_SANITY_SYS_DELETE_FILE(path.c_str());
    FOSSIL_SANITY_SYS_DELETE_FILE(backup.c_str());
}

FOSSIL_TEST(cpp_test_bluecrab_wrapper_double_open_close)
{
    using fossil::db::BlueCrab;
    const std::string path = "/tmp/bluecrab_wrapper_doubleopen";
    const std::string name = "DoubleOpenDB";
    FOSSIL_SANITY_SYS_DELETE_FILE(path.c_str());
    BlueCrab::create(path, name);

    BlueCrab db;
    db.open(path);
    // Opening again should throw
    bool threw = false;
    try { db.open(path); }
    catch (const std::runtime_error&) { threw = true; }
    ASSUME_ITS_TRUE(threw);

    db.close();
    db.close(); // Should not throw

    FOSSIL_SANITY_SYS_DELETE_FILE(path.c_str());
}

FOSSIL_TEST(cpp_test_bluecrab_wrapper_update_nonexistent)
{
    using fossil::db::BlueCrab;
    const std::string path = "/tmp/bluecrab_wrapper_updatenon";
    const std::string name = "UpdateNonDB";
    FOSSIL_SANITY_SYS_DELETE_FILE(path.c_str());
    BlueCrab::create(path, name);
    BlueCrab db(path);

    bool threw = false;
    try { db.update("nope", "object: { foo: cstr:\"bar\" }"); }
    catch (const std::runtime_error&) { threw = true; }
    ASSUME_ITS_TRUE(threw);

    db.close();
    FOSSIL_SANITY_SYS_DELETE_FILE(path.c_str());
}

FOSSIL_TEST(cpp_test_bluecrab_wrapper_remove_nonexistent)
{
    using fossil::db::BlueCrab;
    const std::string path = "/tmp/bluecrab_wrapper_removenon";
    const std::string name = "RemoveNonDB";
    FOSSIL_SANITY_SYS_DELETE_FILE(path.c_str());
    BlueCrab::create(path, name);
    BlueCrab db(path);

    bool threw = false;
    try { db.remove("ghost"); }
    catch (const std::runtime_error&) { threw = true; }
    ASSUME_ITS_TRUE(threw);

    db.close();
    FOSSIL_SANITY_SYS_DELETE_FILE(path.c_str());
}

FOSSIL_TEST(cpp_test_bluecrab_wrapper_insert_sub_nonexistent_parent)
{
    using fossil::db::BlueCrab;
    const std::string path = "/tmp/bluecrab_wrapper_subnonparent";
    const std::string name = "SubNonParentDB";
    FOSSIL_SANITY_SYS_DELETE_FILE(path.c_str());
    BlueCrab::create(path, name);
    BlueCrab db(path);

    bool threw = false;
    try { db.insert_sub("no_parent", "sub", "object: { foo: cstr:\"bar\" }"); }
    catch (const std::runtime_error&) { threw = true; }
    ASSUME_ITS_TRUE(threw);

    db.close();
    FOSSIL_SANITY_SYS_DELETE_FILE(path.c_str());
}

FOSSIL_TEST(cpp_test_bluecrab_wrapper_get_sub_nonexistent)
{
    using fossil::db::BlueCrab;
    const std::string path = "/tmp/bluecrab_wrapper_getsubnon";
    const std::string name = "GetSubNonDB";
    FOSSIL_SANITY_SYS_DELETE_FILE(path.c_str());
    BlueCrab::create(path, name);
    BlueCrab db(path);

    db.insert("parent", "object: { }");
    bool threw = false;
    try { db.get_sub("parent", "no_sub"); }
    catch (const std::runtime_error&) { threw = true; }
    ASSUME_ITS_TRUE(threw);

    db.close();
    FOSSIL_SANITY_SYS_DELETE_FILE(path.c_str());
}

FOSSIL_TEST(cpp_test_bluecrab_wrapper_link_nonexistent)
{
    using fossil::db::BlueCrab;
    const std::string path = "/tmp/bluecrab_wrapper_linknon";
    const std::string name = "LinkNonDB";
    FOSSIL_SANITY_SYS_DELETE_FILE(path.c_str());
    BlueCrab::create(path, name);
    BlueCrab db(path);

    db.insert("A", "object: { }");
    bool threw = false;
    try { db.link("A", "B", "friend"); }
    catch (const std::runtime_error&) { threw = true; }
    ASSUME_ITS_TRUE(threw);

    db.close();
    FOSSIL_SANITY_SYS_DELETE_FILE(path.c_str());
}

FOSSIL_TEST(cpp_test_bluecrab_wrapper_unlink_nonexistent)
{
    using fossil::db::BlueCrab;
    const std::string path = "/tmp/bluecrab_wrapper_unlinknon";
    const std::string name = "UnlinkNonDB";
    FOSSIL_SANITY_SYS_DELETE_FILE(path.c_str());
    BlueCrab::create(path, name);
    BlueCrab db(path);

    db.insert("A", "object: { }");
    db.insert("B", "object: { }");
    // Unlinking a non-existent link should throw
    bool threw = false;
    try { db.unlink("A", "B"); }
    catch (const std::runtime_error&) { threw = true; }
    ASSUME_ITS_TRUE(threw);

    db.close();
    FOSSIL_SANITY_SYS_DELETE_FILE(path.c_str());
}

FOSSIL_TEST(cpp_test_bluecrab_wrapper_search_empty)
{
    using fossil::db::BlueCrab;
    const std::string path = "/tmp/bluecrab_wrapper_searchempty";
    const std::string name = "SearchEmptyDB";
    FOSSIL_SANITY_SYS_DELETE_FILE(path.c_str());
    BlueCrab::create(path, name);
    BlueCrab db(path);

    auto exact = db.search_exact("field", "value");
    ASSUME_ITS_EQUAL_SIZE(exact.size(), 0);

    auto fuzzy = db.search_fuzzy("nothing");
    ASSUME_ITS_EQUAL_SIZE(fuzzy.size(), 0);

    db.close();
    FOSSIL_SANITY_SYS_DELETE_FILE(path.c_str());
}

FOSSIL_TEST(cpp_test_bluecrab_wrapper_hash_consistency)
{
    using fossil::db::BlueCrab;
    const std::string path = "/tmp/bluecrab_wrapper_hashcons";
    const std::string name = "HashConsDB";
    FOSSIL_SANITY_SYS_DELETE_FILE(path.c_str());
    BlueCrab::create(path, name);
    BlueCrab db(path);

    std::string data = "object: { foo: cstr:\"bar\" }";
    std::string hash1 = db.hash_entry(data);
    std::string hash2 = db.hash_entry(data);
    ASSUME_ITS_TRUE(hash1 == hash2);

    db.close();
    FOSSIL_SANITY_SYS_DELETE_FILE(path.c_str());
}

FOSSIL_TEST(cpp_test_bluecrab_wrapper_commit_without_changes)
{
    using fossil::db::BlueCrab;
    const std::string path = "/tmp/bluecrab_wrapper_commitempty";
    const std::string name = "CommitEmptyDB";
    FOSSIL_SANITY_SYS_DELETE_FILE(path.c_str());
    BlueCrab::create(path, name);
    BlueCrab db(path);

    // Committing with no changes should not throw
    db.commit("No changes");

    db.close();
    FOSSIL_SANITY_SYS_DELETE_FILE(path.c_str());
}

// * * * * * * * * * * * * * * * * * * * * * * * *
// * Fossil Logic Test Pool
// * * * * * * * * * * * * * * * * * * * * * * * *
FOSSIL_TEST_GROUP(cpp_bluecrab_database_tests)
{
    FOSSIL_TEST_ADD(cpp_bluecrab_fixture, cpp_test_bluecrab_wrapper_create_open_close);
    FOSSIL_TEST_ADD(cpp_bluecrab_fixture, cpp_test_bluecrab_wrapper_crud);
    FOSSIL_TEST_ADD(cpp_bluecrab_fixture, cpp_test_bluecrab_wrapper_subentry);
    FOSSIL_TEST_ADD(cpp_bluecrab_fixture, cpp_test_bluecrab_wrapper_link_unlink);
    FOSSIL_TEST_ADD(cpp_bluecrab_fixture, cpp_test_bluecrab_wrapper_search);
    FOSSIL_TEST_ADD(cpp_bluecrab_fixture, cpp_test_bluecrab_wrapper_hash_and_verify);
    FOSSIL_TEST_ADD(cpp_bluecrab_fixture, cpp_test_bluecrab_wrapper_commit_log_checkout);
    FOSSIL_TEST_ADD(cpp_bluecrab_fixture, cpp_test_bluecrab_wrapper_compact_verify_backup_restore);
    FOSSIL_TEST_ADD(cpp_bluecrab_fixture, cpp_test_bluecrab_wrapper_double_open_close);
    FOSSIL_TEST_ADD(cpp_bluecrab_fixture, cpp_test_bluecrab_wrapper_update_nonexistent);
    FOSSIL_TEST_ADD(cpp_bluecrab_fixture, cpp_test_bluecrab_wrapper_remove_nonexistent);
    FOSSIL_TEST_ADD(cpp_bluecrab_fixture, cpp_test_bluecrab_wrapper_insert_sub_nonexistent_parent);
    FOSSIL_TEST_ADD(cpp_bluecrab_fixture, cpp_test_bluecrab_wrapper_get_sub_nonexistent);
    FOSSIL_TEST_ADD(cpp_bluecrab_fixture, cpp_test_bluecrab_wrapper_link_nonexistent);
    FOSSIL_TEST_ADD(cpp_bluecrab_fixture, cpp_test_bluecrab_wrapper_unlink_nonexistent);
    FOSSIL_TEST_ADD(cpp_bluecrab_fixture, cpp_test_bluecrab_wrapper_search_empty);
    FOSSIL_TEST_ADD(cpp_bluecrab_fixture, cpp_test_bluecrab_wrapper_hash_consistency);
    FOSSIL_TEST_ADD(cpp_bluecrab_fixture, cpp_test_bluecrab_wrapper_commit_without_changes);

    FOSSIL_TEST_REGISTER(cpp_bluecrab_fixture);
} // end of tests
