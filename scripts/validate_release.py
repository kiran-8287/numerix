#!/usr/bin/env python3
"""Run local release checks, retaining actual command outputs and exit statuses."""
import argparse
import json
import os
from pathlib import Path
import shutil
import subprocess
import tempfile


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--cmake", default="cmake")
    parser.add_argument("--ctest", default="ctest")
    parser.add_argument("--gcc", default="gcc")
    parser.add_argument("--clang", default="clang")
    parser.add_argument("--output", type=Path, default=Path("validation-results"))
    parser.add_argument("--analyzer-gcc", default="gcc")
    parser.add_argument("--cppcheck", default="cppcheck")
    args = parser.parse_args()
    args.output.mkdir(parents=True, exist_ok=True)
    results = []

    def run(name, command, environment=None, input_text=None):
        log = args.output / (name + ".log")
        print(name, flush=True)
        try:
            with log.open("w") as output:
                output.write("Command: " + repr(command) + "\n")
                output.flush()
                process = subprocess.run(command, stdout=output, stderr=subprocess.STDOUT,
                                         env=environment, input=input_text, text=True)
            log.write_text("\n".join(line.rstrip() for line in log.read_text().splitlines()) + "\n")
            status = "Passed" if process.returncode == 0 else "Failed"
            if "analyzer" in name and process.returncode == 0:
                if "warning:" in log.read_text():
                    status = "Partially passed"
            results.append({"name": name, "status": status, "exit_code": process.returncode,
                            "command": command, "log": log.name})
            return process.returncode == 0
        except OSError as error:
            log.write_text(str(error) + "\n")
            results.append({"name": name, "status": "Unavailable", "command": command,
                            "reason": str(error)})
            return False

    for name, flags in [("debug", "-O0 -g3 -Werror"), ("release", "-O3 -DNDEBUG -Werror")]:
        if run("make_" + name + "_clean", ["make", "clean"]):
            run("make_" + name, ["make", "-j2", "all", "test", "CC=" + args.gcc,
                                 "CFLAGS=" + flags])
    for name, compiler in [("gcc", args.gcc), ("clang", args.clang)]:
        if shutil.which(compiler) is None:
            results.append({"name": name, "status": "Unavailable"})
            continue
        for mode in ["Debug", "Release"]:
            tag = name + "_" + mode.lower()
            build = "cmake-build-validation-" + tag
            if run(tag + "_configure", [args.cmake, "-S", ".", "-B", build,
                    "-DCMAKE_C_COMPILER=" + compiler, "-DCMAKE_BUILD_TYPE=" + mode,
                    "-DCMAKE_C_FLAGS=-Werror"]):
                if run(tag + "_build", [args.cmake, "--build", build, "--clean-first", "-j2"]):
                    run(tag + "_ctest", [args.cmake, "-E", "chdir", build, args.ctest, "--output-on-failure"])
    build = "cmake-build-validation-sanitizers"
    environment = dict(os.environ, ASAN_OPTIONS="detect_leaks=1:halt_on_error=1",
                       UBSAN_OPTIONS="halt_on_error=1:print_stacktrace=1")
    if run("sanitizers_configure", [args.cmake, "-S", ".", "-B", build,
            "-DCMAKE_C_COMPILER=" + args.clang, "-DCMAKE_BUILD_TYPE=Debug",
            "-DNUMERIX_BUILD_COMPARISON_BENCHMARK=ON",
            "-DNUMERIX_BENCHMARK_ALLOCATIONS=ON", "-DCMAKE_DISABLE_FIND_PACKAGE_GSL=TRUE",
            "-DCMAKE_C_FLAGS=-O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer"]):
        if run("sanitizers_build", [args.cmake, "--build", build, "--clean-first", "-j2"]):
            run("sanitizers_ctest", [args.cmake, "-E", "chdir", build, args.ctest, "--output-on-failure"], environment)
    tests = ["numerix_tests", "numerix_phase1_tests", "numerix_expression_tests",
             "numerix_audit_regression_tests", "numerix_piecewise_tests",
             "numerix_intelligence_tests", "numerix_execution_tests", "numerix_diagnostics_tests",
             "numerix_root_discovery_tests", "numerix_phase8_integration_tests",
             "numerix_release_regression_tests", "numerix_accuracy_tests"]
    release = Path("cmake-build-validation-gcc_debug")
    if shutil.which("valgrind"):
        for executable in tests:
            extra = []
            # These executables run all categories if no selector is supplied.
            run("valgrind_" + executable, ["valgrind", "--leak-check=full",
                "--show-leak-kinds=all", "--errors-for-leak-kinds=all", "--error-exitcode=99",
                str(release / executable)] + extra)
    else:
        results.append({"name": "valgrind", "status": "Unavailable"})
    # Per-translation-unit analysis; warnings are retained for manual triage.
    analyzer_directory = tempfile.TemporaryDirectory(prefix="numerix-release-analyzer-")
    for source in sorted(Path("src").rglob("*.c")):
        run("clang_analyzer_" + source.stem, [args.clang, "--analyze", "-std=c11",
            "-Iinclude", "-Isrc/internal", "-Xanalyzer", "-analyzer-output=text", str(source)])
        run("gcc_analyzer_" + source.stem, [args.analyzer_gcc, "-fanalyzer", "-std=c11",
            "-Wall", "-Wextra", "-Iinclude", "-Isrc/internal", "-c", str(source), "-o", str(Path(analyzer_directory.name) / (source.stem + ".o"))])
    analyzer_directory.cleanup()
    if shutil.which(args.cppcheck):
        run("cppcheck", [args.cppcheck, "--check-level=exhaustive", "--enable=warning,performance,portability", "--std=c11",
                         "--error-exitcode=1", "-Iinclude", "-Isrc/internal", "src"])
    else:
        results.append({"name": "cppcheck", "status": "Unavailable"})
    results.extend([
        {"name": "Windows native runtime", "status": "Not attempted" if os.name == "nt" else "Unavailable"},
        {"name": "MSVC", "status": "Not attempted" if shutil.which("cl") else "Unavailable"},
    ])
    (args.output / "results.json").write_text(json.dumps(results, indent=2) + "\n")
    print("Validation commands complete; analyzer diagnostics require manual review.")
    if any(result["status"] == "Failed" for result in results):
        raise SystemExit(1)


if __name__ == "__main__":
    main()
