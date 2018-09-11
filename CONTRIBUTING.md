Code Style Guide
================
Please adhere to the following code style and conventions in your pull requests:
Feel free to fix code that doesn't adhere to this guide (yet).

General C++ code style
----------------------
- Use clang-format with the .clang-format style in the repository before you commit.
    There is a script "apply-clang-format.sh" which you can use to fix all files in the repository.
    If you need a section untouched by clang-format you can use 
    ```
    // clang-format off
    some()
        ("fragile", DSL_LIKE)
        (code) / (that - should - not - be - touched);
    // clang-format on
    ```
- Lean towards [Python PEP8 naming conventions](https://www.python.org/dev/peps/pep-0008/#naming-conventions):
    - classes are capitalized `CamelCase`,
    - member functions, free functions and variables are lowercase `snake_case`,
    - constants are uppercase `SNAKE_CASE`.
- Prefix member variables with an `m_`.<br/>
    Member functions should not be prefixed.
- Prefix global variables (if you need them) with a `g_`.
- Use JavaDoc style comments for inline documentation.
- Use `#pragma once` to guard headers/includes.
- Always use braces around the block after control flow statements (`if`,`else`,loops).
- Declare/define only one variable per line.
- Write all code in a namespace, never pollute the global namespace (e.g. with `using namespace` in a header).
- Do not use the global random number generators from the C/C++ libraries.<br/>
    Use a local deterministic PRNG like `std::mt19937` instead or ask the caller for a source of randomness.
- Prefer C++ style casts (`static_cast`, `dynamic_cast`, `const_cast`, `reinterpret_cast`) over C-style casts.
- Never write new.<br/>
    Use C++14 `make_unique`/`make_shared` or
    at least immediately catch the resulting pointer in a `unique_ptr` or `shared_ptr`.
- Include what you need - Headers only include what is needed for the interface of the class.<br/>
    Implementations include what is needed for the implementation.
    Headers SHOULD NOT include stuff that is only needed for the implementation.
    If you want to reduce includes in headers even further, consider using the PIMPL pattern.
- Make helper functions static or put them into an anonymous namespace so that users and the compiler/linker
    know they are local to the compilation unit.
- Write tests.


Specific utilities and conventions in this codebase:
----------------------------------------------------
- All code goes into or below the `::tin-terrain` namespace.
- The use of sub-namespaces below `::tin-terrain` is discouraged.
- Use the `TNTN_ASSERT()` macro from `"tntn/tntn_assert.h"` instead of `assert()` so that the behaviour of asserts can
    be changed when needed.
- Use the `println(...)` and `print(...)` functions from `"tntn/println.h"` to write intentional output to stdout.
    Intentional output could be processed by a script and creates a contract.
- Use `TNTN_LOG_<level>` macros from `"tntn/logging.h"` to output diagnostic message on the commandline.<br/>
    - `TNTN_LOG_ERROR` and `TNTN_LOG_WARN` are message that require special attention from the user.
    - `TNTN_LOG_INFO` are messages that a normal user should see on the console when running a commandline tool.
    - `TNTN_LOG_DEBUG` are messages that only developers should see.
    - `TNTN_LOG_TRACE` are messages that are only available in Debug builds and should only be relevant when you work
       on the specific module.
- Use `File` and `FileLike` classes to read and write files (instead of iostreams or stdio).


Conventions for tests:
----------------------
- Write tests.
- All test code goes into the `::tin-terrain::unittests` namespace.
- The test source file is named after the file where code under test lives.<br/>
    E.g. when you add a feature or fix something in `SomeFile.h/.cpp`, the tests go to `SomeFile_tests.cpp`.
- Never write to the real filesystem in tests, use `MemoryFile` instead.
- Try to avoid reading data from the real filesystem in tests. If you really need to read data for tests
    (e.g. reference data), before you use the real filesystem, consider:
    - generating synthetic test data on the fly instead of using real data
    - the copyright/database rights of assets/resources before you include them in the code repository
    - to include small resources statically in the code by dumping the data into C++ source,
      e.g. with xxd: `xxd -i foo.bin > foo.cpp`


Conventions for file names:
---------------------------
- Use `.h` for header files, `.cpp` for implementations.
- A header file should declare one important/major class.<br/>
    Associated small helper classes can be put into the same header.
- Header files that contain mostly classes should be named after the important/major class inside.<br/>
    E.g. if a header contains the `class Foo` and `class FooHelper` then the header should be named `Foo.h`.
    If a header contains multiple equally important classes
    (not good, but might be appropriate because they belong together), then the file should be also named
    capitalized CamelCase like classes but with a descriptive name.
    E.g. a header that contains `class FooReader` and `class FooWriter` should be named `Foo.h`.
- Header files that are collections of things or contain mostly free functions should be named lowercase snake case.<br/>
    Choose a name that describes the grouping of the contents.
    E.g. if a header contains the functions `concatenate_strings(...)` and `concatenate_vector(...)`,
    a good choice might be `concatenation.h`
- Implementations for headers go into a `.cpp` with the same name.


Git:
--------------------------
- Development should happen on feature branches. Never commit to master directly.
- Branch names should be `kabab-cased.author-nickname` (http://wiki.c2.com/?KebabCase).
- Submit pull request to get stuff merged to master.
- Commit messages should contain meaningful one-line title and optional description after the first line.
- Never merge master to feature branches, rebase your feature branch on top of master instead.

git hooks
=========
If you want to make sure to run clang-format before every commit you do,
you can use the apply-clang-format script as a pre-commit hook in git.
Paste this into `.git/hooks/pre-commit`:
```
#!/bin/sh

./apply-clang-format.sh

```
