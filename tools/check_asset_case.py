"""Finds asset paths written with a different letter case from the file on disk.

Windows doesn't care, but the web build and packed assets do. Each game's asset folder is checked against the paths
and script imports in the tracked files that can use it: the game's, and the engine's.

    python tools/check_asset_case.py

Exit code 0 when every path matches.
"""
import pathlib
import re
import subprocess
import sys

ROOT = pathlib.Path(__file__).resolve().parent.parent


def tracked_files():
    listing = subprocess.run(["git", "ls-files", "-z"], cwd=ROOT, capture_output=True, check=True).stdout
    return [name for name in listing.decode("utf-8").split("\0") if name]


def check_folder(tracked, prefix, mismatches):
    """prefix: the game's folder."""
    asset_prefix = prefix + "assets/"
    assets = {path[len(asset_prefix):] for path in tracked if path.startswith(asset_prefix)}
    folders = set()
    for path in assets:
        parts = path.split("/")
        for count in range(1, len(parts)):
            folders.add("/".join(parts[:count]))
    scripts = {path[len("Scripts/"):-len(".wren")] for path in assets
               if path.startswith("Scripts/") and path.endswith(".wren")}
    files_by_lower = {name.lower(): name for name in assets}
    folders_by_lower = {name.lower(): name for name in folders}
    scripts_by_lower = {name.lower(): name for name in scripts}

    # The game's own files and the engine's code; other games' files are for their own assets.
    sources = [path for path in tracked
               if path.startswith(prefix) and path.endswith((".wren", ".json", ".ldtk", ".cpp", ".h"))]
    sources += [path for path in tracked if path.startswith("src/") and path.endswith((".cpp", ".h"))]
    for source in sources:
        text = (ROOT / source).read_text(encoding="utf-8", errors="ignore")
        imports = set()
        if source.endswith(".wren"):
            imports = set(re.findall(r'^\s*import "([^"]+)"', text, re.M))
        for found in imports:
            actual = scripts_by_lower.get(found.lower())
            if actual and actual != found:
                mismatches.setdefault((found, actual), set()).add(source)
        for found in re.findall(r'"((?:\.\./)*[A-Za-z0-9_ .&()\'-]+/[A-Za-z0-9_ .&()\'/-]*)"', text):
            name = re.sub(r"^(\.\./)+", "", found).rstrip("/")
            if found in imports or name in assets or name in folders:
                continue
            actual = files_by_lower.get(name.lower())
            # A folder name alone only counts in game data: C++ uses similar strings as resource names.
            if not actual and source.startswith(asset_prefix):
                actual = folders_by_lower.get(name.lower())
            if actual:
                mismatches.setdefault((name, actual), set()).add(source)


def main():
    tracked = tracked_files()
    games = sorted({"/".join(path.split("/")[:2]) + "/" for path in tracked
                    if path.startswith("games/") and path.split("/")[2:3] == ["assets"]})
    mismatches = {}
    for prefix in games:
        check_folder(tracked, prefix, mismatches)

    for (found, actual), where in sorted(mismatches.items()):
        print("'%s' should be '%s'  (%s)" % (found, actual, ", ".join(sorted(where)[:3])))
    print("%d paths with the wrong case" % len(mismatches))
    return 1 if mismatches else 0


if __name__ == "__main__":
    sys.exit(main())
