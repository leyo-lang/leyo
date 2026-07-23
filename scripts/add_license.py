from pathlib import Path

HEADER = """/*
 * Copyright (c) 2026 Leyo Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

"""

# Directories to skip
SKIP_DIRS = {
    ".git",
    "build",
    "bin",
    "archive",
}

# File extensions to process
EXTENSIONS = {
    ".c",
    ".h",
}


def should_skip(path: Path) -> bool:
    """Return True if the file is inside a directory we should skip."""
    return any(part in SKIP_DIRS for part in path.parts)


def add_header(path: Path) -> bool:
    """Add the header if it isn't already present."""
    try:
        text = path.read_text(encoding="utf-8")
    except UnicodeDecodeError:
        print(f"SKIP (not UTF-8): {path}")
        return False
    except OSError as e:
        print(f"ERROR reading {path}: {e}")
        return False

    # Don't add another header if one already exists
    if "SPDX-License-Identifier:" in text or "Copyright" in text:
        print(f"SKIP (already has copyright/licence): {path}")
        return False

    try:
        path.write_text(
            HEADER + text,
            encoding="utf-8",
            newline=""
        )
    except OSError as e:
        print(f"ERROR writing {path}: {e}")
        return False

    print(f"UPDATED: {path}")
    return True


def main():
    root = Path.cwd()

    updated = 0
    skipped = 0

    print(f"Scanning: {root}")
    print()

    for path in root.rglob("*"):
        if not path.is_file():
            continue

        if path.suffix.lower() not in EXTENSIONS:
            continue

        if should_skip(path):
            print(f"SKIP (directory excluded): {path}")
            skipped += 1
            continue

        if add_header(path):
            updated += 1
        else:
            skipped += 1

    print()
    print("Done.")
    print(f"Updated: {updated}")
    print(f"Skipped: {skipped}")


if __name__ == "__main__":
    main()