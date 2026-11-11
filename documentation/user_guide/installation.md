# Build, install and consume

A C11 compiler, CMake 3.16+ or GNU Make, and the platform C/math runtime are
required. GSL and Python are **not** required to build/use the static library.
Python 3.9+ is used only by optional benchmark/validation/source-packaging scripts.
See [compatibility](compatibility.md) and [tolerances](../api/tolerances.md).

```sh
cmake -S . -B cmake-build-release -DCMAKE_BUILD_TYPE=Release
cmake --build cmake-build-release --parallel 2
cmake -E chdir cmake-build-release ctest --output-on-failure
cmake --install cmake-build-release --prefix /path/to/numerix
```

Multi-configuration Windows generators additionally use `--config Debug` or
`--config Release` for build/install, and `-C Debug` or `-C Release` for CTest.
No native Windows result is implied by these instructions. MSVC must support
C11 atomics; CMake checks the necessary `/experimental:c11atomics` flag/header.

The installation contains the static archive, numerix.h, the relocatable CMake
package/target **NUMERIX::numerix**, pkg-config metadata, license and user/API/
architecture documentation. Normal installs do not depend on or export GSL.

Independent CMake client:

```cmake
cmake_minimum_required(VERSION 3.16)
project(MyNumericalClient LANGUAGES C)
find_package(NUMERIX 0.2 CONFIG REQUIRED)
add_executable(my_program main.c)
target_link_libraries(my_program PRIVATE NUMERIX::numerix)
```

Configure the client with `-DCMAKE_PREFIX_PATH=/path/to/numerix`; do not add
NUMERIX's src/ files or development include paths. POSIX math linkage propagates
through the imported target. A full C/C++ client is in tests/installed_consumer.
Moving a normal relative-layout prefix preserves package discovery/linkage.

For pkg-config, add the installed lib/pkgconfig or lib64/pkgconfig directory to
PKG_CONFIG_PATH, then use `pkg-config --cflags --libs --static numerix`.
Manual POSIX linking is `cc -std=c11 -I/path/include main.c /path/lib/libnumerix.a -lm`.

Make supports `make`, `make test` and POSIX/MSYS2 `make install PREFIX=/path` with
optional DESTDIR staging. This basic Make install supports direct archive/header
consumption; use CMake installation for exported CMake/pkg-config metadata. Run
`make clean` before changing compilers/flags. Native Windows can use CMake install.

Source packages contain the files needed to build/test/examples/optional benchmarks,
not local builds, credentials, IDE state, raw environment logs or unvalidated
binaries. Historical development reports/raw measurements remain in the repository.
Extract a source archive, follow the commands above, and use SOURCE_MANIFEST.txt
to inspect its contents. Source packaging itself uses scripts/package_source.py;
CPack creates a separate binary archive for the actually validated build platform.

The optional validate_release.py runner writes to ignored validation-results/ by
default, preserving historical committed measurements. Supply --output to retain
an explicitly named new run. CTest3.16 can run from its build directory using
cmake -E chdir, as in the commands above.
