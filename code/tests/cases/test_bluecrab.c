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

FOSSIL_SETUP(c_bluecrab_fixture)
{
    // Setup the test fixture
}

FOSSIL_TEARDOWN(c_bluecrab_fixture)
{
    // Teardown the test fixture
}

// * * * * * * * * * * * * * * * * * * * * * * * *
// * Fossil Logic Test Blue CrabDB Database
// * * * * * * * * * * * * * * * * * * * * * * * *

#define TEST_DB_PATH "/tmp/bluecrab_testdb"
#define TEST_DB_NAME "TestDB"

FOSSIL_TEST(c_test_bluecrab_create_open_close_delete)
{
    FOSSIL_SANITY_SYS_DELETE_FILE(TEST_DB_PATH);
    ASSUME_ITS_TRUE(fossil_db_bluecrab_create(TEST_DB_PATH, TEST_DB_NAME) == 0);

    fossil_bluecrab_db db;
    ASSUME_ITS_TRUE(fossil_db_bluecrab_open(&db, TEST_DB_PATH) == 0);
    ASSUME_ITS_TRUE(db.opened);

    ASSUME_ITS_TRUE(fossil_db_bluecrab_close(&db) == 0);

    // Ensure file is closed before attempting to delete, and retry if needed
    int del_result = fossil_db_bluecrab_delete(TEST_DB_PATH);
    if (del_result != 0) {
        // Wait briefly and try again in case of OS file lock delay
        FOSSIL_SANITY_SYS_SLEEP(1000); // 1 second
        del_result = fossil_db_bluecrab_delete(TEST_DB_PATH);
    }
    ASSUME_ITS_TRUE(del_result == 0);
}

FOSSIL_TEST(c_test_bluecrab_crud_entry)
{
    FOSSIL_SANITY_SYS_DELETE_FILE(TEST_DB_PATH);
    ASSUME_ITS_TRUE(fossil_db_bluecrab_create(TEST_DB_PATH, TEST_DB_NAME) == 0);
    fossil_bluecrab_db db;
    ASSUME_ITS_TRUE(fossil_db_bluecrab_open(&db, TEST_DB_PATH) == 0);

    const char *id = "entry1";
    const char *fson = "object: { name: cstr:\"Alice\", age: i32:30 }";
    char *out = NULL;

    ASSUME_ITS_TRUE(fossil_db_bluecrab_insert(&db, id, fson) == 0);
    ASSUME_ITS_TRUE(fossil_db_bluecrab_get(&db, id, &out) == 0);
    ASSUME_NOT_CNULL(out);
    ASSUME_ITS_CSTR_CONTAINS(out, "Alice");
    free(out);

    const char *fson2 = "object: { name: cstr:\"Bob\", age: i32:40 }";
    ASSUME_ITS_TRUE(fossil_db_bluecrab_update(&db, id, fson2) == 0);
    ASSUME_ITS_TRUE(fossil_db_bluecrab_get(&db, id, &out) == 0);
    ASSUME_NOT_CNULL(out);
    ASSUME_ITS_CSTR_CONTAINS(out, "Bob");
    free(out);

    ASSUME_ITS_TRUE(fossil_db_bluecrab_remove(&db, id) == 0);
    ASSUME_NOT_TRUE(fossil_db_bluecrab_get(&db, id, &out) == 0);

    ASSUME_ITS_TRUE(fossil_db_bluecrab_close(&db) == 0);
    ASSUME_ITS_TRUE(fossil_db_bluecrab_delete(TEST_DB_PATH) == 0);
}

FOSSIL_TEST(c_test_bluecrab_subentry)
{
    FOSSIL_SANITY_SYS_DELETE_FILE(TEST_DB_PATH);
    fossil_db_bluecrab_create(TEST_DB_PATH, TEST_DB_NAME);
    fossil_bluecrab_db db;
    fossil_db_bluecrab_open(&db, TEST_DB_PATH);

    const char *parent = "parent";
    const char *sub = "child";
    const char *fson = "object: { value: i32:123 }";
    char *out = NULL;

    ASSUME_ITS_TRUE(fossil_db_bluecrab_insert_sub(&db, parent, sub, fson) == 0);
    ASSUME_ITS_TRUE(fossil_db_bluecrab_get_sub(&db, parent, sub, &out) == 0);
    ASSUME_NOT_CNULL(out);
    ASSUME_ITS_CSTR_CONTAINS(out, "123");
    free(out);

    fossil_db_bluecrab_close(&db);
    fossil_db_bluecrab_delete(TEST_DB_PATH);
}

FOSSIL_TEST(c_test_bluecrab_relations)
{
    FOSSIL_SANITY_SYS_DELETE_FILE(TEST_DB_PATH);
    ASSUME_ITS_TRUE(fossil_db_bluecrab_create(TEST_DB_PATH, TEST_DB_NAME) == 0);
    fossil_bluecrab_db db;
    ASSUME_ITS_TRUE(fossil_db_bluecrab_open(&db, TEST_DB_PATH) == 0);

    ASSUME_ITS_TRUE(fossil_db_bluecrab_insert(&db, "A", "object: { }") == 0);
    ASSUME_ITS_TRUE(fossil_db_bluecrab_insert(&db, "B", "object: { }") == 0);
    ASSUME_ITS_TRUE(fossil_db_bluecrab_insert(&db, "C", "object: { }") == 0);

    ASSUME_ITS_TRUE(fossil_db_bluecrab_link(&db, "A", "B", "friend") == 0);
    ASSUME_ITS_TRUE(fossil_db_bluecrab_link(&db, "A", "C", "colleague") == 0);

    fossil_bluecrab_relation *rels = NULL;
    size_t count = 0;
    ASSUME_ITS_TRUE(fossil_db_bluecrab_get_relations(&db, "A", &rels, &count) == 0);
    ASSUME_ITS_MORE_OR_EQUAL_SIZE(count, 2);
    free(rels);

    ASSUME_ITS_TRUE(fossil_db_bluecrab_unlink(&db, "A", "B") == 0);
    ASSUME_ITS_TRUE(fossil_db_bluecrab_get_relations(&db, "A", &rels, &count) == 0);
    ASSUME_ITS_MORE_OR_EQUAL_SIZE(count, 1);
    free(rels);

    ASSUME_ITS_TRUE(fossil_db_bluecrab_close(&db) == 0);
    ASSUME_ITS_TRUE(fossil_db_bluecrab_delete(TEST_DB_PATH) == 0);
}

FOSSIL_TEST(c_test_bluecrab_search_exact_and_fuzzy)
{
    FOSSIL_SANITY_SYS_DELETE_FILE(TEST_DB_PATH);
    ASSUME_ITS_TRUE(fossil_db_bluecrab_create(TEST_DB_PATH, TEST_DB_NAME) == 0);
    fossil_bluecrab_db db;
    ASSUME_ITS_TRUE(fossil_db_bluecrab_open(&db, TEST_DB_PATH) == 0);

    ASSUME_ITS_TRUE(fossil_db_bluecrab_insert(&db, "e1", "object: { name: cstr:\"Alpha\", tag: cstr:\"red\" }") == 0);
    ASSUME_ITS_TRUE(fossil_db_bluecrab_insert(&db, "e2", "object: { name: cstr:\"Beta\", tag: cstr:\"blue\" }") == 0);
    ASSUME_ITS_TRUE(fossil_db_bluecrab_insert(&db, "e3", "object: { name: cstr:\"Gamma\", tag: cstr:\"red\" }") == 0);

    fossil_bluecrab_search_result *results = NULL;
    size_t count = 0;

    ASSUME_ITS_TRUE(fossil_db_bluecrab_search_exact(&db, "tag", "red", &results, &count) == 0);
    ASSUME_ITS_EQUAL_SIZE(count, 2);
    free(results);

    results = NULL;
    count = 0;
    ASSUME_ITS_TRUE(fossil_db_bluecrab_search_fuzzy(&db, "Alpha", &results, &count) == 0);
    if (count == 0) {
        // Try lowercase search in case the fuzzy search is case-sensitive
        ASSUME_ITS_TRUE(fossil_db_bluecrab_search_fuzzy(&db, "alpha", &results, &count) == 0);
    }
    ASSUME_ITS_EQUAL_SIZE(count, 1);
    free(results);

    ASSUME_ITS_TRUE(fossil_db_bluecrab_close(&db) == 0);
    ASSUME_ITS_TRUE(fossil_db_bluecrab_delete(TEST_DB_PATH) == 0);
}

FOSSIL_TEST(c_test_bluecrab_hash_and_verify)
{
    FOSSIL_SANITY_SYS_DELETE_FILE(TEST_DB_PATH);
    fossil_db_bluecrab_create(TEST_DB_PATH, TEST_DB_NAME);
    fossil_bluecrab_db db;
    fossil_db_bluecrab_open(&db, TEST_DB_PATH);

    const char *id = "hashentry";
    const char *fson = "object: { foo: cstr:\"bar\" }";
    fossil_db_bluecrab_insert(&db, id, fson);

    char *data = NULL;
    char hash[FOSSIL_BLUECRAB_HASH_SIZE];
    ASSUME_ITS_TRUE(fossil_db_bluecrab_get(&db, id, &data) == 0);
    ASSUME_NOT_CNULL(data);
    ASSUME_ITS_TRUE(fossil_db_bluecrab_hash_entry(data, hash) == 0);
    free(data);

    ASSUME_ITS_TRUE(fossil_db_bluecrab_verify_entry(&db, id) == 0);

    fossil_db_bluecrab_close(&db);
    fossil_db_bluecrab_delete(TEST_DB_PATH);
}

FOSSIL_TEST(c_test_bluecrab_commit_log_checkout)
{
    FOSSIL_SANITY_SYS_DELETE_FILE(TEST_DB_PATH);
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

FOSSIL_TEST(c_test_bluecrab_meta_and_advanced)
{
    FOSSIL_SANITY_SYS_DELETE_FILE(TEST_DB_PATH);
    fossil_db_bluecrab_create(TEST_DB_PATH, TEST_DB_NAME);
    fossil_bluecrab_db db;
    fossil_db_bluecrab_open(&db, TEST_DB_PATH);

    ASSUME_ITS_TRUE(fossil_db_bluecrab_meta_load(&db) == 0);
    ASSUME_ITS_TRUE(fossil_db_bluecrab_meta_save(&db) == 0);
    ASSUME_ITS_TRUE(fossil_db_bluecrab_meta_rebuild(&db) == 0);

    // Backup and restore (basic smoke test)
    const char *backup_path = "/tmp/bluecrab_testdb_backup";
    FOSSIL_SANITY_SYS_DELETE_FILE(backup_path);
    ASSUME_ITS_TRUE(fossil_db_bluecrab_backup(&db, backup_path) == 0);
    ASSUME_ITS_TRUE(fossil_db_bluecrab_restore(&db, backup_path) == 0);

    // Compact and verify
    ASSUME_ITS_TRUE(fossil_db_bluecrab_compact(&db) == 0);
    ASSUME_ITS_TRUE(fossil_db_bluecrab_verify(&db) == 0);

    fossil_db_bluecrab_close(&db);
    fossil_db_bluecrab_delete(TEST_DB_PATH);
    FOSSIL_SANITY_SYS_DELETE_FILE(backup_path);
}

FOSSIL_TEST(c_test_bluecrab_multiple_entries_and_bulk_crud)
{
    FOSSIL_SANITY_SYS_DELETE_FILE(TEST_DB_PATH);
    ASSUME_ITS_TRUE(fossil_db_bluecrab_create(TEST_DB_PATH, TEST_DB_NAME) == 0);
    fossil_bluecrab_db db;
    ASSUME_ITS_TRUE(fossil_db_bluecrab_open(&db, TEST_DB_PATH) == 0);

    // Insert multiple entries
    const char *ids[] = {"id1", "id2", "id3", "id4"};
    const char *fsons[] = {
        "object: { name: cstr:\"One\", value: i32:1 }",
        "object: { name: cstr:\"Two\", value: i32:2 }",
        "object: { name: cstr:\"Three\", value: i32:3 }",
        "object: { name: cstr:\"Four\", value: i32:4 }"
    };
    char *out = NULL;
    for (int i = 0; i < 4; ++i) {
        ASSUME_ITS_TRUE(fossil_db_bluecrab_insert(&db, ids[i], fsons[i]) == 0);
        ASSUME_ITS_TRUE(fossil_db_bluecrab_get(&db, ids[i], &out) == 0);
        ASSUME_NOT_CNULL(out);
        ASSUME_ITS_CSTR_CONTAINS(out, "object");
        free(out);
    }

    // Remove all entries
    for (int i = 0; i < 4; ++i) {
        ASSUME_ITS_TRUE(fossil_db_bluecrab_remove(&db, ids[i]) == 0);
    }

    // Confirm removal
    for (int i = 0; i < 4; ++i) {
        ASSUME_NOT_TRUE(fossil_db_bluecrab_get(&db, ids[i], &out) == 0);
    }

    fossil_db_bluecrab_close(&db);
    fossil_db_bluecrab_delete(TEST_DB_PATH);
}

FOSSIL_TEST(c_test_bluecrab_subentry_update_and_remove)
{
    FOSSIL_SANITY_SYS_DELETE_FILE(TEST_DB_PATH);
    fossil_db_bluecrab_create(TEST_DB_PATH, TEST_DB_NAME);
    fossil_bluecrab_db db;
    fossil_db_bluecrab_open(&db, TEST_DB_PATH);

    const char *parent = "parent2";
    const char *sub = "child2";
    const char *fson = "object: { value: i32:555 }";
    char *out = NULL;

    ASSUME_ITS_TRUE(fossil_db_bluecrab_insert_sub(&db, parent, sub, fson) == 0);
    ASSUME_ITS_TRUE(fossil_db_bluecrab_get_sub(&db, parent, sub, &out) == 0);
    ASSUME_NOT_CNULL(out);
    ASSUME_ITS_CSTR_CONTAINS(out, "555");
    free(out);

    // Update subentry
    const char *fson2 = "object: { value: i32:777 }";
    char id[FOSSIL_BLUECRAB_MAX_ID * 2];
    snprintf(id, sizeof(id), "%s_%s", parent, sub);
    ASSUME_ITS_TRUE(fossil_db_bluecrab_update(&db, id, fson2) == 0);
    ASSUME_ITS_TRUE(fossil_db_bluecrab_get_sub(&db, parent, sub, &out) == 0);
    ASSUME_ITS_CSTR_CONTAINS(out, "777");
    free(out);

    // Remove subentry
    ASSUME_ITS_TRUE(fossil_db_bluecrab_remove(&db, id) == 0);
    ASSUME_NOT_TRUE(fossil_db_bluecrab_get_sub(&db, parent, sub, &out) == 0);

    fossil_db_bluecrab_close(&db);
    fossil_db_bluecrab_delete(TEST_DB_PATH);
}

FOSSIL_TEST(c_test_bluecrab_relation_metadata_and_types)
{
    FOSSIL_SANITY_SYS_DELETE_FILE(TEST_DB_PATH);
    fossil_db_bluecrab_create(TEST_DB_PATH, TEST_DB_NAME);
    fossil_bluecrab_db db;
    fossil_db_bluecrab_open(&db, TEST_DB_PATH);

    ASSUME_ITS_TRUE(fossil_db_bluecrab_insert(&db, "X", "object: { }") == 0);
    ASSUME_ITS_TRUE(fossil_db_bluecrab_insert(&db, "Y", "object: { }") == 0);

    // Link with different relation types
    ASSUME_ITS_TRUE(fossil_db_bluecrab_link(&db, "X", "Y", "parent") == 0);
    ASSUME_ITS_TRUE(fossil_db_bluecrab_link(&db, "Y", "X", "child") == 0);

    fossil_bluecrab_relation *rels = NULL;
    size_t count = 0;
    ASSUME_ITS_TRUE(fossil_db_bluecrab_get_relations(&db, "X", &rels, &count) == 0);
    ASSUME_ITS_MORE_OR_EQUAL_SIZE(count, 2);
    int found_parent = 0, found_child = 0;
    for (size_t i = 0; i < count; ++i) {
        if (strcmp(rels[i].relation_type, "parent") == 0) found_parent = 1;
        if (strcmp(rels[i].relation_type, "child") == 0) found_child = 1;
    }
    ASSUME_ITS_TRUE(found_parent && found_child);
    free(rels);

    fossil_db_bluecrab_close(&db);
    fossil_db_bluecrab_delete(TEST_DB_PATH);
}

FOSSIL_TEST(c_test_bluecrab_search_no_results_and_empty_db)
{
    FOSSIL_SANITY_SYS_DELETE_FILE(TEST_DB_PATH);
    fossil_db_bluecrab_create(TEST_DB_PATH, TEST_DB_NAME);
    fossil_bluecrab_db db;
    fossil_db_bluecrab_open(&db, TEST_DB_PATH);

    fossil_bluecrab_search_result *results = NULL;
    size_t count = 0;

    // Search in empty DB
    ASSUME_ITS_TRUE(fossil_db_bluecrab_search_exact(&db, "field", "value", &results, &count) == 0);
    ASSUME_ITS_EQUAL_SIZE(count, 0);
    free(results);

    // Insert unrelated entry
    fossil_db_bluecrab_insert(&db, "foo", "object: { name: cstr:\"bar\" }");
    ASSUME_ITS_TRUE(fossil_db_bluecrab_search_exact(&db, "name", "baz", &results, &count) == 0);
    ASSUME_ITS_EQUAL_SIZE(count, 0);
    free(results);

    // Fuzzy search with no match
    ASSUME_ITS_TRUE(fossil_db_bluecrab_search_fuzzy(&db, "notfound", &results, &count) == 0);
    ASSUME_ITS_EQUAL_SIZE(count, 0);
    free(results);

    fossil_db_bluecrab_close(&db);
    fossil_db_bluecrab_delete(TEST_DB_PATH);
}

FOSSIL_TEST(c_test_bluecrab_commit_and_checkout_invalid_version)
{
    FOSSIL_SANITY_SYS_DELETE_FILE(TEST_DB_PATH);
    fossil_db_bluecrab_create(TEST_DB_PATH, TEST_DB_NAME);
    fossil_bluecrab_db db;
    fossil_db_bluecrab_open(&db, TEST_DB_PATH);

    fossil_db_bluecrab_insert(&db, "v1", "object: { v: i32:10 }");
    fossil_db_bluecrab_commit(&db, "Commit 1");

    // Try to checkout a non-existent version
    ASSUME_NOT_TRUE(fossil_db_bluecrab_checkout(&db, "9999") == 0);

    fossil_db_bluecrab_close(&db);
    fossil_db_bluecrab_delete(TEST_DB_PATH);
}

FOSSIL_TEST(c_test_bluecrab_hash_consistency)
{
    FOSSIL_SANITY_SYS_DELETE_FILE(TEST_DB_PATH);
    fossil_db_bluecrab_create(TEST_DB_PATH, TEST_DB_NAME);
    fossil_bluecrab_db db;
    fossil_db_bluecrab_open(&db, TEST_DB_PATH);

    const char *id = "hashcheck";
    const char *fson = "object: { foo: cstr:\"baz\" }";
    fossil_db_bluecrab_insert(&db, id, fson);

    char *data1 = NULL;
    char hash1[FOSSIL_BLUECRAB_HASH_SIZE];
    ASSUME_ITS_TRUE(fossil_db_bluecrab_get(&db, id, &data1) == 0);
    ASSUME_ITS_TRUE(fossil_db_bluecrab_hash_entry(data1, hash1) == 0);

    // Hash again, should be the same
    char hash2[FOSSIL_BLUECRAB_HASH_SIZE];
    ASSUME_ITS_TRUE(fossil_db_bluecrab_hash_entry(data1, hash2) == 0);
    ASSUME_ITS_EQUAL_CSTR(hash1, hash2);
    free(data1);

    fossil_db_bluecrab_close(&db);
    fossil_db_bluecrab_delete(TEST_DB_PATH);
}

FOSSIL_TEST(c_test_bluecrab_bulk_insert_and_fuzzy_rank)
{
    FOSSIL_SANITY_SYS_DELETE_FILE(TEST_DB_PATH);
    ASSUME_ITS_TRUE(fossil_db_bluecrab_create(TEST_DB_PATH, TEST_DB_NAME) == 0);
    fossil_bluecrab_db db;
    ASSUME_ITS_TRUE(fossil_db_bluecrab_open(&db, TEST_DB_PATH) == 0);

    // Bulk insert 100 entries with similar and distinct names
    char id[32], fson[128];
    for (int i = 0; i < 100; ++i) {
        snprintf(id, sizeof(id), "user_%02d", i);
        snprintf(fson, sizeof(fson), "object: { name: cstr:\"User%02d\", tag: cstr:\"%s\" }", i, (i % 2 == 0) ? "even" : "odd");
        ASSUME_ITS_TRUE(fossil_db_bluecrab_insert(&db, id, fson) == 0);
    }

    // Fuzzy search for "User1" (should match User10, User11, User12, etc.)
    fossil_bluecrab_search_result *results = NULL;
    size_t count = 0;
    ASSUME_ITS_TRUE(fossil_db_bluecrab_search_fuzzy(&db, "User1", &results, &count) == 0);
    ASSUME_ITS_EQUAL_SIZE(count, 10);

    // Rank results and check that the top result is "user_10" or "user_11"
    ASSUME_ITS_TRUE(fossil_db_bluecrab_rank_results(results, count) == 0);
    int top_is_expected = (strcmp(results[0].id, "user_10") == 0 || strcmp(results[0].id, "user_11") == 0);
    ASSUME_ITS_TRUE(top_is_expected);
    free(results);

    ASSUME_ITS_TRUE(fossil_db_bluecrab_close(&db) == 0);
    ASSUME_ITS_TRUE(fossil_db_bluecrab_delete(TEST_DB_PATH) == 0);
}

FOSSIL_TEST(c_test_bluecrab_dag_cycle_prevention)
{
    FOSSIL_SANITY_SYS_DELETE_FILE(TEST_DB_PATH);
    ASSUME_ITS_TRUE(fossil_db_bluecrab_create(TEST_DB_PATH, TEST_DB_NAME) == 0);
    fossil_bluecrab_db db;
    ASSUME_ITS_TRUE(fossil_db_bluecrab_open(&db, TEST_DB_PATH) == 0);

    // Insert three nodes
    ASSUME_ITS_TRUE(fossil_db_bluecrab_insert(&db, "A", "object: { }") == 0);
    ASSUME_ITS_TRUE(fossil_db_bluecrab_insert(&db, "B", "object: { }") == 0);
    ASSUME_ITS_TRUE(fossil_db_bluecrab_insert(&db, "C", "object: { }") == 0);

    // Create a DAG: A -> B -> C
    ASSUME_ITS_TRUE(fossil_db_bluecrab_link(&db, "A", "B", "edge") == 0);
    ASSUME_ITS_TRUE(fossil_db_bluecrab_link(&db, "B", "C", "edge") == 0);

    // Attempt to create a cycle: C -> A (should fail)
    ASSUME_NOT_TRUE(fossil_db_bluecrab_link(&db, "C", "A", "edge") == 0);

    ASSUME_ITS_TRUE(fossil_db_bluecrab_close(&db) == 0);
    ASSUME_ITS_TRUE(fossil_db_bluecrab_delete(TEST_DB_PATH) == 0);
}

FOSSIL_TEST(c_test_bluecrab_subentry_bulk_and_removal)
{
    FOSSIL_SANITY_SYS_DELETE_FILE(TEST_DB_PATH);
    ASSUME_ITS_TRUE(fossil_db_bluecrab_create(TEST_DB_PATH, TEST_DB_NAME) == 0);
    fossil_bluecrab_db db;
    ASSUME_ITS_TRUE(fossil_db_bluecrab_open(&db, TEST_DB_PATH) == 0);

    // Insert parent entry
    const char *parent = "parent_bulk";
    ASSUME_ITS_TRUE(fossil_db_bluecrab_insert(&db, parent, "object: { }") == 0);

    // Insert 20 sub-entries
    char subid[32], fson[64];
    for (int i = 0; i < 20; ++i) {
        snprintf(subid, sizeof(subid), "sub_%02d", i);
        snprintf(fson, sizeof(fson), "object: { value: i32:%d }", i * 10);
        ASSUME_ITS_TRUE(fossil_db_bluecrab_insert_sub(&db, parent, subid, fson) == 0);
    }

    // Remove all sub-entries
    for (int i = 0; i < 20; ++i) {
        snprintf(subid, sizeof(subid), "sub_%02d", i);
        char fullid[FOSSIL_BLUECRAB_MAX_ID * 2];
        snprintf(fullid, sizeof(fullid), "%s_%s", parent, subid);
        ASSUME_ITS_TRUE(fossil_db_bluecrab_remove(&db, fullid) == 0);
    }

    // Confirm removal
    char *out = NULL;
    for (int i = 0; i < 20; ++i) {
        snprintf(subid, sizeof(subid), "sub_%02d", i);
        ASSUME_NOT_TRUE(fossil_db_bluecrab_get_sub(&db, parent, subid, &out) == 0);
    }

    ASSUME_ITS_TRUE(fossil_db_bluecrab_close(&db) == 0);
    ASSUME_ITS_TRUE(fossil_db_bluecrab_delete(TEST_DB_PATH) == 0);
}

FOSSIL_TEST(c_test_bluecrab_backup_restore_integrity)
{
    FOSSIL_SANITY_SYS_DELETE_FILE(TEST_DB_PATH);
    ASSUME_ITS_TRUE(fossil_db_bluecrab_create(TEST_DB_PATH, TEST_DB_NAME) == 0);
    fossil_bluecrab_db db;
    ASSUME_ITS_TRUE(fossil_db_bluecrab_open(&db, TEST_DB_PATH) == 0);

    // Insert entries
    ASSUME_ITS_TRUE(fossil_db_bluecrab_insert(&db, "bk1", "object: { foo: cstr:\"bar\" }") == 0);
    ASSUME_ITS_TRUE(fossil_db_bluecrab_insert(&db, "bk2", "object: { foo: cstr:\"baz\" }") == 0);

    // Backup
    const char *backup_path = "/tmp/bluecrab_testdb_adv_backup";
    FOSSIL_SANITY_SYS_DELETE_FILE(backup_path);
    ASSUME_ITS_TRUE(fossil_db_bluecrab_backup(&db, backup_path) == 0);

    // Remove one entry and verify it's gone
    ASSUME_ITS_TRUE(fossil_db_bluecrab_remove(&db, "bk2") == 0);
    char *out = NULL;
    ASSUME_NOT_TRUE(fossil_db_bluecrab_get(&db, "bk2", &out) == 0);

    // Restore from backup and verify both entries exist
    ASSUME_ITS_TRUE(fossil_db_bluecrab_restore(&db, backup_path) == 0);
    ASSUME_ITS_TRUE(fossil_db_bluecrab_get(&db, "bk1", &out) == 0);
    free(out);
    ASSUME_ITS_TRUE(fossil_db_bluecrab_get(&db, "bk2", &out) == 0);
    free(out);

    ASSUME_ITS_TRUE(fossil_db_bluecrab_close(&db) == 0);
    ASSUME_ITS_TRUE(fossil_db_bluecrab_delete(TEST_DB_PATH) == 0);
    FOSSIL_SANITY_SYS_DELETE_FILE(backup_path);
}

FOSSIL_TEST(c_test_bluecrab_similarity_and_ranking)
{
    // Test the similarity scoring and ranking helpers
    float sim1 = fossil_db_bluecrab_similarity("Alpha", "Alpha");
    float sim2 = fossil_db_bluecrab_similarity("Alpha", "Alfa");
    float sim3 = fossil_db_bluecrab_similarity("Alpha", "Beta");
    ASSUME_ITS_MORE_THAN_F32(sim1, sim2);
    ASSUME_ITS_MORE_THAN_F32(sim2, sim3);

    fossil_bluecrab_search_result results[3] = {
        {.id = "A", .score = sim1},
        {.id = "B", .score = sim2},
        {.id = "C", .score = sim3}
    };
    ASSUME_ITS_TRUE(fossil_db_bluecrab_rank_results(results, 3) == 0);
    ASSUME_ITS_EQUAL_CSTR(results[0].id, "A");
    ASSUME_ITS_EQUAL_CSTR(results[1].id, "B");
    ASSUME_ITS_EQUAL_CSTR(results[2].id, "C");
}

// * * * * * * * * * * * * * * * * * * * * * * * *
// * Fossil Logic Test Pool
// * * * * * * * * * * * * * * * * * * * * * * * *
FOSSIL_TEST_GROUP(c_bluecrab_database_tests)
{
    FOSSIL_TEST_ADD(c_bluecrab_fixture, c_test_bluecrab_create_open_close_delete);
    FOSSIL_TEST_ADD(c_bluecrab_fixture, c_test_bluecrab_crud_entry);
    FOSSIL_TEST_ADD(c_bluecrab_fixture, c_test_bluecrab_subentry);
    FOSSIL_TEST_ADD(c_bluecrab_fixture, c_test_bluecrab_relations);
    FOSSIL_TEST_ADD(c_bluecrab_fixture, c_test_bluecrab_search_exact_and_fuzzy);
    FOSSIL_TEST_ADD(c_bluecrab_fixture, c_test_bluecrab_hash_and_verify);
    FOSSIL_TEST_ADD(c_bluecrab_fixture, c_test_bluecrab_commit_log_checkout);
    FOSSIL_TEST_ADD(c_bluecrab_fixture, c_test_bluecrab_meta_and_advanced);
    FOSSIL_TEST_ADD(c_bluecrab_fixture, c_test_bluecrab_multiple_entries_and_bulk_crud);
    FOSSIL_TEST_ADD(c_bluecrab_fixture, c_test_bluecrab_subentry_update_and_remove);
    FOSSIL_TEST_ADD(c_bluecrab_fixture, c_test_bluecrab_relation_metadata_and_types);
    FOSSIL_TEST_ADD(c_bluecrab_fixture, c_test_bluecrab_search_no_results_and_empty_db);
    FOSSIL_TEST_ADD(c_bluecrab_fixture, c_test_bluecrab_commit_and_checkout_invalid_version);
    FOSSIL_TEST_ADD(c_bluecrab_fixture, c_test_bluecrab_hash_consistency);
    FOSSIL_TEST_ADD(c_bluecrab_fixture, c_test_bluecrab_bulk_insert_and_fuzzy_rank);
    FOSSIL_TEST_ADD(c_bluecrab_fixture, c_test_bluecrab_dag_cycle_prevention);
    FOSSIL_TEST_ADD(c_bluecrab_fixture, c_test_bluecrab_subentry_bulk_and_removal);
    FOSSIL_TEST_ADD(c_bluecrab_fixture, c_test_bluecrab_backup_restore_integrity);
    FOSSIL_TEST_ADD(c_bluecrab_fixture, c_test_bluecrab_similarity_and_ranking);

    FOSSIL_TEST_REGISTER(c_bluecrab_fixture);
} // end of tests
