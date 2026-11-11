#!/usr/bin/env python3
"""Create a reproducible, source-only candidate archive from reviewed source files."""
import argparse
import gzip
import hashlib
import io
from pathlib import Path
import re
import subprocess
import tarfile


ROOT_FILES = {"CMakeLists.txt", "Makefile", "LICENSE", "README.md", "CHANGELOG.md", ".gitignore"}
PREFIXES = ("src/", "include/", "cli/", "examples/", "tests/", "cmake/", "scripts/",
            ".github/workflows/", "documentation/api/", "documentation/architecture/",
            "documentation/user_guide/")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source-dir", type=Path, default=Path(__file__).resolve().parents[1])
    parser.add_argument("--output", type=Path, default=Path("dist"))
    args = parser.parse_args()
    root = args.source_dir.resolve()
    try:
        tracked = subprocess.check_output(["git", "-C", str(root), "ls-files", "-z"],
                                          stderr=subprocess.DEVNULL).decode().split("\0")
    except (subprocess.CalledProcessError, FileNotFoundError):
        tracked = (root / "SOURCE_MANIFEST.txt").read_text().splitlines()
    names = sorted({name for name in tracked if name in ROOT_FILES or name.startswith(PREFIXES)
                    or name.startswith("benchmarks/") and not name.startswith("benchmarks/results/")})
    version = re.search(r'NUMERIX_VERSION_STRING\s+"([\w.\-]+)"',
                        (root / "include/numerix.h").read_text()).group(1)
    prefix = "numerix-" + version + "/"
    args.output.mkdir(parents=True, exist_ok=True)
    destination = args.output / ("numerix-" + version + "-source.tar.gz")
    manifest = ("\n".join(sorted(names + ["SOURCE_MANIFEST.txt"])) + "\n").encode()
    with destination.open("wb") as output:
        with gzip.GzipFile(filename="", mode="wb", fileobj=output, mtime=0) as compressed:
            with tarfile.open(fileobj=compressed, mode="w", format=tarfile.PAX_FORMAT) as archive:
                for name in sorted(names + ["SOURCE_MANIFEST.txt"]):
                    path = root / name
                    if Path(name).is_absolute() or ".." in Path(name).parts:
                        raise ValueError("Source manifest contains an unsafe path: " + name)
                    if name == "SOURCE_MANIFEST.txt":
                        content = manifest
                    else:
                        if path.is_symlink() or not path.resolve().is_relative_to(root):
                            raise ValueError("Source path must be a regular contained file: " + name)
                        content = path.read_bytes()
                    entry = tarfile.TarInfo(prefix + name)
                    entry.size = len(content)
                    entry.mode = 0o644
                    entry.mtime = 0
                    archive.addfile(entry, io.BytesIO(content))
    checksum = hashlib.sha256(destination.read_bytes()).hexdigest()
    destination.with_name(destination.name + ".sha256").write_text(checksum + "  " + destination.name + "\n")
    print(destination)
    print("SHA256 " + checksum)


if __name__ == "__main__":
    main()
