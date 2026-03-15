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

FOSSIL_SUITE(c_bluecrab_fixture);

FOSSIL_SETUP(c_bluecrab_fixture) {
    // Setup the test fixture
}

FOSSIL_TEARDOWN(c_bluecrab_fixture) {
    // Teardown the test fixture
}

// * * * * * * * * * * * * * * * * * * * * * * * *
// * Fossil Logic Test Blue CrabDB Database
// * * * * * * * * * * * * * * * * * * * * * * * *

#define TEST_DB_PATH "/tmp/bluecrab_testdb"
#define TEST_DB_NAME "TestDB"

FOSSIL_TEST(c_test_bluecrab_create_open_close_delete) {
    remove(TEST_DB_PATH);
    ASSUME_ITS_TRUE(fossil_db_bluecrab_create(TEST_DB_PATH, TEST_DB_NAME) == 0);

    fossil_bluecrab_db db;
    ASSUME_ITS_TRUE(fossil_db_bluecrab_open(&db, TEST_DB_PATH) == 0);
    ASSUME_ITS_TRUE(db.opened);

    ASSUME_ITS_TRUE(fossil_db_bluecrab_close(&db) == 0);
    ASSUME_ITS_TRUE(fossil_db_bluecrab_delete(TEST_DB_PATH) == 0);
}

FOSSIL_TEST(c_test_bluecrab_crud_entry) {
    remove(TEST_DB_PATH);
    fossil_db_bluecrab_create(TEST_DB_PATH, TEST_DB_NAME);
    fossil_bluecrab_db db;
    fossil_db_bluecrab_open(&db, TEST_DB_PATH);

    const char *id = "entry1";
    const char *fson = "object: { name: cstr:\"Alice\", age: i32:30 }";
    char *out = NULL;

    ASSUME_ITS_TRUE(fossil_db_bluecrab_insert(&db, id, fson) == 0);
    ASSUME_ITS_TRUE(fossil_db_bluecrab_get(&db, id, &out) == 0);
    ASSUME_ITS_TRUE(out && strstr(out, "Alice"));
    free(out);

    const char *fson2 = "object: { name: cstr:\"Bob\", age: i32:40 }";
    ASSUME_ITS_TRUE(fossil_db_bluecrab_update(&db, id, fson2) == 0);
    ASSUME_ITS_TRUE(fossil_db_bluecrab_get(&db, id, &out) == 0);
    ASSUME_ITS_TRUE(out && strstr(out, "Bob"));
    free(out);

    ASSUME_ITS_TRUE(fossil_db_bluecrab_remove(&db, id) == 0);
    ASSUME_ITS_TRUE(fossil_db_bluecrab_get(&db, id, &out) != 0);

    fossil_db_bluecrab_close(&db);
    fossil_db_bluecrab_delete(TEST_DB_PATH);
}

FOSSIL_TEST(c_test_bluecrab_subentry) {
    remove(TEST_DB_PATH);
    fossil_db_bluecrab_create(TEST_DB_PATH, TEST_DB_NAME);
    fossil_bluecrab_db db;
    fossil_db_bluecrab_open(&db, TEST_DB_PATH);

    const char *parent = "parent";
    const char *sub = "child";
    const char *fson = "object: { value: i32:123 }";
    char *out = NULL;

    ASSUME_ITS_TRUE(fossil_db_bluecrab_insert_sub(&db, parent, sub, fson) == 0);
    ASSUME_ITS_TRUE(fossil_db_bluecrab_get_sub(&db, parent, sub, &out) == 0);
    ASSUME_ITS_TRUE(out && strstr(out, "123"));
    free(out);

    fossil_db_bluecrab_close(&db);
    fossil_db_bluecrab_delete(TEST_DB_PATH);
}

FOSSIL_TEST(c_test_bluecrab_relations) {
    remove(TEST_DB_PATH);
    fossil_db_bluecrab_create(TEST_DB_PATH, TEST_DB_NAME);
    fossil_bluecrab_db db;
    fossil_db_bluecrab_open(&db, TEST_DB_PATH);

    fossil_db_bluecrab_insert(&db, "A", "object: { }");
    fossil_db_bluecrab_insert(&db, "B", "object: { }");
    fossil_db_bluecrab_insert(&db, "C", "object: { }");

    ASSUME_ITS_TRUE(fossil_db_bluecrab_link(&db, "A", "B", "friend") == 0);
    ASSUME_ITS_TRUE(fossil_db_bluecrab_link(&db, "A", "C", "colleague") == 0);

    fossil_bluecrab_relation *rels = NULL;
    size_t count = 0;
    ASSUME_ITS_TRUE(fossil_db_bluecrab_get_relations(&db, "A", &rels, &count) == 0);
    ASSUME_ITS_TRUE(count >= 2);
    free(rels);

    ASSUME_ITS_TRUE(fossil_db_bluecrab_unlink(&db, "A", "B") == 0);
    ASSUME_ITS_TRUE(fossil_db_bluecrab_get_relations(&db, "A", &rels, &count) == 0);
    ASSUME_ITS_TRUE(count >= 1);
    free(rels);

    fossil_db_bluecrab_close(&db);
    fossil_db_bluecrab_delete(TEST_DB_PATH);
}

FOSSIL_TEST(c_test_bluecrab_search_exact_and_fuzzy) {
    remove(TEST_DB_PATH);
    fossil_db_bluecrab_create(TEST_DB_PATH, TEST_DB_NAME);
    fossil_bluecrab_db db;
    fossil_db_bluecrab_open(&db, TEST_DB_PATH);

    fossil_db_bluecrab_insert(&db, "e1", "object: { name: cstr:\"Alpha\", tag: cstr:\"red\" }");
    fossil_db_bluecrab_insert(&db, "e2", "object: { name: cstr:\"Beta\", tag: cstr:\"blue\" }");
    fossil_db_bluecrab_insert(&db, "e3", "object: { name: cstr:\"Gamma\", tag: cstr:\"red\" }");

    fossil_bluecrab_search_result *results = NULL;
    size_t count = 0;

    ASSUME_ITS_TRUE(fossil_db_bluecrab_search_exact(&db, "tag", "red", &results, &count) == 0);
    ASSUME_ITS_TRUE(count == 2);
    free(results);

    ASSUME_ITS_TRUE(fossil_db_bluecrab_search_fuzzy(&db, "Alpha", &results, &count) == 0);
    ASSUME_ITS_TRUE(count >= 1);
    free(results);

    fossil_db_bluecrab_close(&db);
    fossil_db_bluecrab_delete(TEST_DB_PATH);
}

FOSSIL_TEST(c_test_bluecrab_hash_and_verify) {
    remove(TEST_DB_PATH);
    fossil_db_bluecrab_create(TEST_DB_PATH, TEST_DB_NAME);
    fossil_bluecrab_db db;
    fossil_db_bluecrab_open(&db, TEST_DB_PATH);

    const char *id = "hashentry";
    const char *fson = "object: { foo: cstr:\"bar\" }";
    fossil_db_bluecrab_insert(&db, id, fson);

    char *data = NULL;
    char hash[FOSSIL_BLUECRAB_HASH_SIZE];
    ASSUME_ITS_TRUE(fossil_db_bluecrab_get(&db, id, &data) == 0);
    ASSUME_ITS_TRUE(fossil_db_bluecrab_hash_entry(data, hash) == 0);
    free(data);

    ASSUME_ITS_TRUE(fossil_db_bluecrab_verify_entry(&db, id) == 0);

    fossil_db_bluecrab_close(&db);
    fossil_db_bluecrab_delete(TEST_DB_PATH);
}

FOSSIL_TEST(c_test_bluecrab_commit_log_checkout) {
    remove(TEST_DB_PATH);
    fossil_db_bluecrab_create(TEST_DB_PATH, TEST_DB_NAME);
    fossil_bluecrab_db db;
    fossil_db_bluecrab_open(&db, TEST_DB_PATH);

    fossil_db_bluecrab_insert(&db, "c1", "object: { v: i32:1 }");
    ASSUME_ITS_TRUE(fossil_db_bluecrab_commit(&db, "Initial commit") == 0);

    fossil_db_bluecrab_insert(&db, "c2", "object: { v: i32:2 }");
    ASSUME_ITS_TRUE(fossil_db_bluecrab_commit(&db, "Second commit") == 0);

    ASSUME_ITS_TRUE(fossil_db_bluecrab_log(&db) == 0);

    // Try checkout (should print commit info)
    ASSUME_ITS_TRUE(fossil_db_bluecrab_checkout(&db, "1") == 0);

    fossil_db_bluecrab_close(&db);
    fossil_db_bluecrab_delete(TEST_DB_PATH);
}

FOSSIL_TEST(c_test_bluecrab_meta_and_advanced) {
    remove(TEST_DB_PATH);
    fossil_db_bluecrab_create(TEST_DB_PATH, TEST_DB_NAME);
    fossil_bluecrab_db db;
    fossil_db_bluecrab_open(&db, TEST_DB_PATH);

    ASSUME_ITS_TRUE(fossil_db_bluecrab_meta_load(&db) == 0);
    ASSUME_ITS_TRUE(fossil_db_bluecrab_meta_save(&db) == 0);
    ASSUME_ITS_TRUE(fossil_db_bluecrab_meta_rebuild(&db) == 0);

    // Backup and restore (basic smoke test)
    const char *backup_path = "/tmp/bluecrab_testdb_backup";
    remove(backup_path);
    ASSUME_ITS_TRUE(fossil_db_bluecrab_backup(&db, backup_path) == 0);
    ASSUME_ITS_TRUE(fossil_db_bluecrab_restore(&db, backup_path) == 0);

    // Compact and verify
    ASSUME_ITS_TRUE(fossil_db_bluecrab_compact(&db) == 0);
    ASSUME_ITS_TRUE(fossil_db_bluecrab_verify(&db) == 0);

    fossil_db_bluecrab_close(&db);
    fossil_db_bluecrab_delete(TEST_DB_PATH);
    remove(backup_path);
}

// * * * * * * * * * * * * * * * * * * * * * * * *
// * Fossil Logic Test Pool
// * * * * * * * * * * * * * * * * * * * * * * * *
FOSSIL_TEST_GROUP(c_bluecrab_database_tests) {
    FOSSIL_TEST_ADD(c_bluecrab_fixture, c_test_bluecrab_create_open_close_delete);
    FOSSIL_TEST_ADD(c_bluecrab_fixture, c_test_bluecrab_crud_entry);
    FOSSIL_TEST_ADD(c_bluecrab_fixture, c_test_bluecrab_subentry);
    FOSSIL_TEST_ADD(c_bluecrab_fixture, c_test_bluecrab_relations);
    FOSSIL_TEST_ADD(c_bluecrab_fixture, c_test_bluecrab_search_exact_and_fuzzy);
    FOSSIL_TEST_ADD(c_bluecrab_fixture, c_test_bluecrab_hash_and_verify);
    FOSSIL_TEST_ADD(c_bluecrab_fixture, c_test_bluecrab_commit_log_checkout);
    FOSSIL_TEST_ADD(c_bluecrab_fixture, c_test_bluecrab_meta_and_advanced);

    FOSSIL_TEST_REGISTER(c_bluecrab_fixture);
} // end of tests
