# CrabDB

CrabDB is a lightweight, embedded C database library with an in-memory table registry, database lifecycle management, typed values, transaction control, and pointer-free file persistence. Its implementation is designed around explicit ownership, dynamically sized collections, and clear status reporting.

## Key Features

- File-backed and in-memory database creation and opening
- Pointer-free persistence of tables, fields, records, and values for file-backed databases
- Dynamic multi-table management, including create, drop, rename, and existence checks
- Internal table, record, field, value, query, result, transaction, and database structures
- Typed values with owned dynamically allocated data
- Transaction begin, commit, and rollback support for table changes
- Result containers with count tracking and cleanup
- Consistent status codes, last-error messages, and bounded error storage
- Record and query APIs prepared for further implementation

## Overview

CrabDB is intended for embedded and application-level use cases where a compact database API is needed without the overhead of a server. A database tracks its path, storage mode, lifecycle state, read-only and transaction state, table registry, affected-row count, status, and a bounded 256-byte error buffer. Tables maintain dynamically sized field and record collections and monotonically increasing record IDs. Records associate named values with their owning table, fields describe typed and constrained columns, values own dynamically allocated data, queries track parsing state, results provide positioned record collections, and transactions track table state.

The current implementation provides database creation, opening, closing, destruction, status reporting, table management, record management, value creation and destruction, transactions, result containers, and persistence. File-backed databases are serialized in a private pointer-free format when closed, while memory databases remain entirely in memory. Records can be inserted with automatically assigned IDs, updated by ID, deleted by pointer or ID, and selected into results containing record pointers and count information. Dynamic storage grows as needed, and insert, update, and delete operations update the database affected-row count. Field and query structures are defined for continued schema and query API development.

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
