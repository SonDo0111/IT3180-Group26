# GEMINI.md - IT3180 Group 26 Project Guidelines

## Project Overview

**IT3180-Group26** is an Apartment Management System developed for the course **IT3180** (Introduction to Software Engineering / Project) at Hanoi University of Science and Technology (HUST).

- **Domain:** Residential apartment and building administration (household management, fee collection, billing, payments, and reporting).
- **Primary Language:** C++ (target standard C++20 / C++17).
- **Build System:** CMake (utilizing CMakeLists.txt and CMakePresets.json).
- **Target OS:** Linux / Cross-platform.

---

## Architecture and Directory Layout

The repository is structured following a modular, layered architectural pattern:

```
IT3180-Group26/
|-- CMakeLists.txt        # Top-level CMake build configuration
|-- CMakePresets.json     # Standardized build presets (Debug, Release, Test)
|-- .clang-format         # LLVM-based code formatting configuration
|-- CHANGELOG.md          # Version history and release notes
|-- README.md             # Project summary and introduction
|-- config/               # Application configuration files (DB, environment, defaults)
|-- db/                   # Database scripts and schemas
|   |-- migrations/       # Schema migrations and DDL scripts
|   `-- seed/             # Initial database seed fixtures
|-- deploy/               # Deployment artifacts
|   |-- packaging/        # Packaging configurations (e.g. deb, rpm, bundle)
|   |-- scripts/          # Deployment and environment provisioning scripts
|   `-- INSTALL.md        # Deployment and installation documentation
|-- docs/                 # Project documentation and specifications
|   |-- architecture.md   # Architectural design, diagrams, and domain boundaries
|   |-- conventions.md    # Development, code review, and team standards
|   `-- sprints/          # Sprint backlogs, planning, and retrospective notes
|-- src/                  # Main C++ source tree
|   |-- app/              # Application entrypoint (main executable, UI/CLI controllers)
|   |-- core/             # Core shared infrastructure (DB abstraction, models, utilities)
|   `-- modules/          # Business domain modules:
|       |-- household/    # Residents, apartments, household registration and status
|       |-- fee/          # Fee definitions (maintenance, parking, utilities, donations)
|       |-- payment/      # Billing invoices, payment processing, transaction records
|       `-- report/       # Statistical reporting, financial summaries, debtor tracking
`-- tests/                # Automated test suites
    |-- unit/             # Unit tests for individual classes/functions
    |-- integration/      # Integration tests (cross-module and database flows)
    |-- fixtures/         # Shared test data and mocks
    |-- test-cases/       # Formal test case definitions and QA specifications
    `-- reports/          # Test execution output and coverage reports
```

### Module Responsibilities

1. **`src/core/`**: Shared foundation components including database connectivity, logging, configuration management, and base domain classes.
2. **`src/modules/household/`**: Handles apartment units, household registries (ho khau), resident profiles (nhan khau), and residency status changes (tam tru, tam vang).
3. **`src/modules/fee/`**: Manages fee policies, mandatory rates (phi dich vu, phi gui xe), and voluntary contributions (quy khuyen hoc, quy vi nguoi ngheo).
4. **`src/modules/payment/`**: Tracks monthly invoices, payment receipts, balance calculation, and overdue statuses.
5. **`src/modules/report/`**: Generates financial statistics, demographic reports, and revenue collection metrics.
6. **`src/app/`**: Composes the modules and serves the user interface (CLI menu or GUI) and application lifecycle.

---

## Building, Running, and Testing

The project uses modern CMake. As concrete target names are implemented in CMakeLists.txt, replace placeholders with actual executable names.

### Prerequisites

- C++ Compiler supporting C++20/C++17 (g++ >= 11 or clang++ >= 13; g++ 14.2 is installed)
- CMake >= 3.20 (CMake 3.31.6 is installed)
- Make or Ninja build system (GNU Make 4.4.1 is installed)
- clang-format (for code formatting)

### Build Commands

```bash
# Configure the build directory (Out-of-source build)
cmake -B build -S . -DCMAKE_BUILD_TYPE=Debug

# Compile the project
cmake --build build

# Alternative using CMake presets (when presets are defined in CMakePresets.json):
# cmake --preset default
# cmake --build --preset default
```

### Running the Application

```bash
# Run the main application (TODO: adjust executable path once target is set in CMakeLists.txt)
./build/src/app/apartment_management
```

### Running Tests

```bash
# Run all automated tests via CTest
ctest --test-dir build --output-on-failure

# Alternatively, run specific test binaries directly once built:
# ./build/tests/unit/unit_tests
# ./build/tests/integration/integration_tests
```

---

## Coding Standards and Conventions

All C++ source files (.cpp, .hpp, .h, .cc) must strictly conform to `.clang-format`:

### Style Rules Summary

- **Base Style:** LLVM
- **Indentation:** 4 spaces (Tabs are forbidden: `UseTab: Never`).
- **Column Limit:** 100 characters.
- **Brace Style:** Stroustrup (`BreakBeforeBraces: Stroustrup`).
- **Pointer/Reference Alignment:** Left-aligned (`Type* ptr;`, `Type& ref;`).
- **Access Modifiers:** Indented with -4 offset relative to class body (aligned with class declaration).
- **Switch Statements:** Indent case labels (`IndentCaseLabels: true`).
- **Control Flow:** Short blocks, if statements, and loops must never be on a single line; explicit braces are preferred.

### Formatting Command

To check and apply code formatting across the repository:

```bash
# Format C++ source and header files
clang-format -i src/**/*.cpp src/**/*.hpp src/**/*.h tests/**/*.cpp
```

### C++ Best Practices

1. **Modern C++ Idioms:**
   - Prefer std::unique_ptr and std::shared_ptr over raw owning pointers.
   - Use std::string_view and std::span for non-owning parameter passing where appropriate.
   - Ensure const-correctness across all member methods and parameters.
   - Mark single-argument constructors as explicit.
   - Prefer scoped enums (enum class) over raw enums.

2. **Error Handling:**
   - Use exceptions for exceptional system failures or std::expected / std::optional for expected domain validation failures.
   - Avoid silent failures or ignoring return values.

3. **Modularity and Boundaries:**
   - Keep headers self-contained with include guards (#pragma once).
   - Modules should expose clean public APIs and hide implementation details in internal headers or .cpp compilation units.
   - Avoid circular dependencies between domain modules; mediate communication via src/core/ services or event interfaces.

4. **Testing Standards:**
   - Add unit tests for every newly introduced business rule in tests/unit/.
   - Use fixtures in tests/fixtures/ for repeatable test states.
   - Ensure all tests pass prior to submitting changes.
