# CrabDB

CrabDB is a lightweight, embedded relational database engine designed for efficient data storage and retrieval in C applications. It provides a comprehensive API for managing databases, tables, records, and queries with a focus on performance, reliability, and simplicity.

## Key Features

- Multi-table database support with flexible schema definition
- Record-based storage with typed field values
- Query support for flexible data retrieval and filtering
- Transaction management for ACID compliance
- Comprehensive error handling and status reporting
- Support for multiple data types including integers, floats, strings, booleans, and more

## Primary Components

- Database: Top-level container for tables and records
- Table: Structured collection of records with defined fields
- Record: Individual data row with typed field values
- Query: Advanced data retrieval and filtering mechanism
- Transaction: Atomic operations ensuring data consistency

## Usage Pattern

1. Create or open a database using `fossil_db_crabdb_create()`
2. Create tables with `fossil_db_crabdb_table_create()`
3. Insert records using `fossil_db_crabdb_record_insert()`
4. Query data using `fossil_db_crabdb_query_*()` functions
5. Manage transactions with `fossil_db_crabdb_transaction_*()` functions
6. Close the database with `fossil_db_crabdb_close()`

## Overview

CrabDB is intended for embedded and application-level use cases where a compact relational database is needed without the overhead of a full server-based database system. It provides the core building blocks for schema-driven storage, efficient queries, and safe transactional behavior in native C programs.

## ***Prerequisites***

To get started, ensure you have the following installed:

- **Meson Build System**: If you don’t have Meson `1.10.0` or newer installed, follow the installation instructions on the official [Meson website](https://mesonbuild.com/Getting-meson.html).

### Adding Dependency

#### Adding via Meson Git Wrap

To add a git-wrap, place a `.wrap` file in `subprojects` with the Git repo URL and revision, then use `dependency('fossil-db')` in `meson.build` so Meson can fetch and build it automatically.

#### Integrate the Dependency:

Add the `fossil-db.wrap` file in your `subprojects` directory and include the following content:

```ini
[wrap-git]
url = https://github.com/fossillogic/fossil-db.git
revision = v1.0.1

[provide]
dependency_names = fossil-db
```

**Note**: For the best experience, always use the latest releases. Visit the [releases](https://github.com/fossillogic/fossil-crabdb/releases) page for the latest versions.

## Configure Options

You have options when configuring the build, each serving a different purpose:

- **Running Tests**: To enable running tests, use `-Dwith_test=enabled` when configuring the build.

Example:

```sh
meson setup builddir -Dwith_test=enabled
```

### Tests Double as Samples

The project is designed so that **test cases serve two purposes**:

- ✅ **Unit Tests** – validate the framework’s correctness.  
- 📖 **Usage Samples** – demonstrate how to use these libraries through test cases.  

This approach keeps the codebase compact and avoids redundant “hello world” style examples.  
Instead, the same code that proves correctness also teaches usage.  

This mirrors the **Meson build system** itself, which tests its own functionality by using Meson to test Meson.  
In the same way, Fossil Logic validates itself by demonstrating real-world usage in its own tests via Fossil Test.  

```bash
meson test -C builddir -v
```

Running the test suite gives you both verification and practical examples you can learn from.

## Contributing and Support

If you're interested in contributing to this project, encounter any issues, have questions, or would like to provide feedback, don't hesitate to open an issue or visit the [Fossil Logic Docs](https://fossillogic.com/docs) for more information.
