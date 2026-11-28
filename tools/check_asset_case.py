"""Finds asset paths written with a different letter case from the file on disk.

Windows doesn't care, but the web build and packed assets do. Each game's asset folder is checked against the paths and
script imports in the tracked files that can use it: the game's, and the engine's.

    python tools/check_asset_case.py <game folder>...        (default: the game repositories beside this one)
    python engine/tools/check_asset_case.py .                (from a game's folder)

Exit code 0 when every path matches.
"""
import pathlib
import re
import subprocess
import sys

ENGINE = pathlib.Path(__file__).resolve().parent.parent
SOURCES = (".wren", ".json", ".ldtk", ".cpp", ".h")


def tracked_files(folder):
    listing = subprocess.run(["git", "ls-files", "-z"], cwd=folder, capture_output=True, check=True).stdout
    return [name for name in listing.decode("utf-8").split("\0") if name]


def check_game(game, engine_sources, mismatches):
    tracked = tracked_files(game)
    assets = {path[len("assets/"):] for path in tracked if path.startswith("assets/")}
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

    sources = [(game, path) for path in tracked if path.endswith(SOURCES) and not path.startswith("third_party/")]
    sources += [(ENGINE, path) for path in engine_sources]
    for folder, source in sources:
        text = (folder / source).read_text(encoding="utf-8", errors="ignore")
        in_assets = folder == game and source.startswith("assets/")
        where = "%s/%s" % (folder.name, source)
        imports = set()
        if source.endswith(".wren"):
            imports = set(re.findall(r'^\s*import "([^"]+)"', text, re.M))
        for found in imports:
            actual = scripts_by_lower.get(found.lower())
            if actual and actual != found:
                mismatches.setdefault((found, actual), set()).add(where)
        for found in re.findall(r'"((?:\.\./)*[A-Za-z0-9_ .&()\'-]+/[A-Za-z0-9_ .&()\'/-]*)"', text):
            name = re.sub(r"^(\.\./)+", "", found).rstrip("/")
            if found in imports or name in assets or name in folders:
                continue
            actual = files_by_lower.get(name.lower())
            # A folder name alone only counts in game data: C++ uses similar strings as resource names.
            if not actual and in_assets:
                actual = folders_by_lower.get(name.lower())
            if actual:
                mismatches.setdefault((name, actual), set()).add(where)


def main(arguments):
    games = [pathlib.Path(argument).resolve() for argument in arguments]
    if not games:
        games = sorted(folder for folder in ENGINE.parent.iterdir()
                       if folder != ENGINE and (folder / ".gitmodules").is_file() and (folder / "assets").is_dir()
                       and "Struktur" in (folder / ".gitmodules").read_text(encoding="utf-8", errors="ignore"))
    if not games:
        raise SystemExit("no game folders: name one or more")
    engine_sources = [path for path in tracked_files(ENGINE)
                      if path.startswith("src/") and path.endswith((".cpp", ".h"))]
    mismatches = {}
    for game in games:
        check_game(game, engine_sources, mismatches)

    for (found, actual), where in sorted(mismatches.items()):
        print("'%s' should be '%s'  (%s)" % (found, actual, ", ".join(sorted(where)[:3])))
    print("%d paths with the wrong case in %s" % (len(mismatches), ", ".join(game.name for game in games)))
    return 1 if mismatches else 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
