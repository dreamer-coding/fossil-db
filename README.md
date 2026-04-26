# **Fossil DB Library by Fossil Logic**

## CrabDB Overview

**CrabDB** is a lightweight, embeddable database engine designed for deterministic behavior, strong data integrity, and minimal operational overhead. Built around a unified internal engine, CrabDB provides structured data storage with version tracking, write-ahead logging (WAL), and portable serialization.

Rather than splitting into separate systems, CrabDB exposes multiple operational modes on top of the same core: a structured query interface, a direct key-value layer, and an optional in-memory cache layer. Every operation flows through the same engine, ensuring consistency, traceability, and reproducibility across all workflows.

At its foundation, CrabDB uses a **log-driven architecture** with deterministic hashing and versioning. Each mutation updates the database state, producing a verifiable root hash and commit record. This enables reliable auditing, rollback, and integrity verification without external tooling.

CrabDB integrates a lightweight structured format (**FSON**) for representing records, allowing both simple key-value storage and more complex, self-describing data without requiring rigid schemas.

---

## Key Features

- **Portability & Cross-Platform**
  - Fully portable C implementation with minimal dependencies
  - Builds cleanly across Linux, macOS, Windows, and embedded targets
  - Consistent behavior across toolchains and compilers

- **Unified Engine Architecture**
  - Single internal engine drives all database operations
  - Subsystems operate on the engine (not the other way around)
  - Clear separation between API layer and execution core

- **Multiple Operational Modes**
  - Structured query mode for FSON-backed record operations
  - Direct key-value mode for low-level CRUD access
  - Cache mode for fast in-memory operations with optional TTL support

- **CRUD Operations**
  - Insert, read, update, and delete across all modes
  - Deterministic state transitions through the engine
  - WAL-backed mutation tracking for durability

- **Versioning & Integrity**
  - Every mutation increments a global version counter
  - Root hash generated from deterministic state folding
  - Supports verification, auditability, and rollback semantics

- **Write-Ahead Logging (WAL)**
  - All changes recorded before final commit
  - Enables crash recovery and replay capability
  - Lightweight append-only log format

- **Backup and Restore**
  - File-level backup with safe copy semantics
  - Atomic restore via temporary staging file replacement
  - Version state preserved across restore operations

- **FSON Structured Data**
  - Self-describing record format for flexible schemas
  - Supports both simple key-value and structured records
  - Enables interoperability across all database modes

- **Memory Management**
  - Explicit allocation and ownership rules
  - Linked-record in-memory index model
  - Optional cache layer with TTL-based cleanup

- **Database Lifecycle Management**
  - Create, open, close, delete, and compact operations
  - Compaction reduces logical fragmentation and refreshes state
  - Verification hooks for integrity checking and corruption detection

---

## Design Principles

- **Engine-Centric Architecture**  
  All functionality is driven by a single internal engine. Higher-level interfaces are thin layers, not separate systems.

- **Deterministic State**  
  Database state is reproducible via versioning and hashing, enabling verification and auditability.

- **Minimal Abstraction Leakage**  
  Subsystems (storage, indexing, WAL) operate on the engine — never the other way around.

- **Portable by Default**  
  Strict C implementation with predictable behavior across platforms and toolchains.

- **Structured Without Complexity**  
  FSON provides flexible, self-describing data without introducing heavy schema enforcement.
## ***Prerequisites***

To get started, ensure you have the following installed:

- **Meson Build System**: If you don’t have Meson `1.8.0` or newer installed, follow the installation instructions on the official [Meson website](https://mesonbuild.com/Getting-meson.html).

### Adding Dependency

#### Adding via Meson Git Wrap

To add a git-wrap, place a `.wrap` file in `subprojects` with the Git repo URL and revision, then use `dependency('fossil-crabdb')` in `meson.build` so Meson can fetch and build it automatically.

#### Integrate the Dependency:

Add the `fossil-crabdb.wrap` file in your `subprojects` directory and include the following content:

```ini
[wrap-git]
url = https://github.com/fossillogic/fossil-db.git
revision = v0.2.5

[provide]
dependency_names = fossil-db
```

**Note**: For the best experience, always use the latest releases. Visit the [releases](https://github.com/fossillogic/fossil-db/releases) page for the latest versions.

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
