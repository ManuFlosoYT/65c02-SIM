#!/bin/bash
set -e

TARGET_OS="linux"
BUILD_TYPE="Release"
CMAKE_OPTS=""
DO_CLEAN=false
DO_RUN=false
FORCE_CONFIGURE=false
MINIMAL_CORE=false
LOCAL_SOURCES=false

while [[ $# -gt 0 ]]; do
    case "$1" in
        --minimal-core)
            MINIMAL_CORE=true
            shift
            ;;
        --local-sources)
            LOCAL_SOURCES=true
            shift
            ;;
        --clean)
            DO_CLEAN=true
            shift
            ;;
        --debug)
            BUILD_TYPE="Debug"
            shift
            ;;
        --release)
            BUILD_TYPE="Release"
            shift
            ;;
        --run)
            DO_RUN=true
            shift
            ;;
        --reconfigure|--configure)
            FORCE_CONFIGURE=true
            shift
            ;;
        --target)
            TARGET_OS="$2"
            shift 2
            ;;
        --target=*)
            TARGET_OS="${1#*=}"
            shift
            ;;
        linux|win|web|flatpak)
            TARGET_OS="$1"
            shift
            ;;
        -h|--help)
            echo "Usage: ./build.sh [OPTIONS] [linux|win|web|flatpak]"
            echo "Options:"
            echo "  --release          Build in Release mode (default)"
            echo "  --debug            Build in Debug mode"
            echo "  --clean            Clean build directory before building"
            echo "  --reconfigure      Force CMake reconfiguration"
            echo "  --minimal-core     Build only minimal headless core for NSF compilation"
            echo "  --local-sources    Build Flatpak from local workspace checkout (for CI)"
            echo "  --run              Run application after build"
            echo "  --target <os>      Target OS: linux, win, web, flatpak"
            echo "  -h, --help         Show this help message"
            exit 0
            ;;
        *)
            echo "Unknown argument: $1"
            exit 1
            ;;
    esac
done

echo "Building for OS: $TARGET_OS in $BUILD_TYPE mode"

# Setup Build Directory based on OS and TYPE
if [ "$MINIMAL_CORE" = true ]; then
    BUILD_DIR="build/nsf_core"
elif [[ "$TARGET_OS" == "win" ]]; then
    if [[ "$BUILD_TYPE" == "Debug" ]]; then BUILD_DIR="build_win/debug"; else BUILD_DIR="build_win/release"; fi
elif [[ "$TARGET_OS" == "web" ]]; then
    BUILD_DIR="build/web"
elif [[ "$TARGET_OS" == "flatpak" ]]; then
    BUILD_DIR="build/flatpak-build"
else # linux default
    if [[ "$BUILD_TYPE" == "Debug" ]]; then BUILD_DIR="build/debug"; else BUILD_DIR="build/release"; fi
fi

if [ "$DO_CLEAN" = true ]; then
    echo "Cleaning build directory ($BUILD_DIR)..."
    rm -rf "$BUILD_DIR"
fi

if command -v ccache >/dev/null 2>&1; then
    echo "ccache found, enabling..."
    export CCACHE_SLOPPINESS="pch_defines,time_macros"
    CMAKE_OPTS="$CMAKE_OPTS -DCMAKE_C_COMPILER_LAUNCHER=ccache -DCMAKE_CXX_COMPILER_LAUNCHER=ccache"
    ccache -M 15G >/dev/null 2>&1 || true
    if [ "$DO_CLEAN" = true ]; then
        ccache -z
    fi
fi

if command -v ninja >/dev/null 2>&1; then
    echo "Ninja found, using it as generator..."
    export CMAKE_GENERATOR=Ninja
    
    if [ -f "$BUILD_DIR/CMakeCache.txt" ]; then
        if grep -q "CMAKE_GENERATOR:INTERNAL=Unix Makefiles" "$BUILD_DIR/CMakeCache.txt"; then
            echo "Generator mismatch detected (Unix Makefiles -> Ninja). Cleaning build directory..."
            rm -rf "$BUILD_DIR"
        fi
    fi
fi

if command -v mold >/dev/null 2>&1; then
    if [[ "$TARGET_OS" == "linux" || "$MINIMAL_CORE" == true ]]; then
        echo "mold found, enabling..."
        CMAKE_OPTS="$CMAKE_OPTS -DCMAKE_LINKER_TYPE=MOLD"
    fi
fi

if [ "$MINIMAL_CORE" = true ]; then
    if [ ! -f "$BUILD_DIR/CMakeCache.txt" ] || [ "$FORCE_CONFIGURE" = true ]; then
        cmake -S . -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE=$BUILD_TYPE -DBUILD_NSF_RUNNER_ONLY=ON $CMAKE_OPTS
    else
        echo "Skipping CMake configure (cache found). Use --reconfigure to force it."
    fi
    cmake --build "$BUILD_DIR" -j$(nproc)
    mkdir -p output
    cp "$BUILD_DIR/SIM_65C02" output/ 2>/dev/null || true
    echo "Minimal NSF core build completed in output/SIM_65C02"
    exit 0
fi

if [[ "$TARGET_OS" == "web" ]]; then
    EMSDK_DIR="emsdk_local"
    if ! command -v emcmake >/dev/null 2>&1; then
        if [ ! -d "$EMSDK_DIR" ]; then
            echo "Emscripten SDK not found. Downloading to $EMSDK_DIR..."
            git clone --depth 1 https://github.com/emscripten-core/emsdk.git "$EMSDK_DIR"
            cd "$EMSDK_DIR"
            ./emsdk install latest
            ./emsdk activate latest
            cd ..
        fi
        echo "Activating Emscripten SDK..."
        source "$EMSDK_DIR/emsdk_env.sh" > /dev/null 2>&1
    fi

    if ! command -v emcmake >/dev/null 2>&1; then
        echo "Error: Failed to activate Emscripten. Please try manual installation."
        exit 1
    fi

    mkdir -p "$BUILD_DIR"
    if [ ! -f "$BUILD_DIR/CMakeCache.txt" ] || [ "$FORCE_CONFIGURE" = true ]; then
        emcmake cmake -S . -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE=$BUILD_TYPE $CMAKE_OPTS
    else
        echo "Skipping CMake configure (cache found). Use --reconfigure to force it."
    fi
    cmake --build "$BUILD_DIR" -j$(nproc)

    mkdir -p output/web
    cp "$BUILD_DIR/SIM_65C02.html" output/web/index.html 2>/dev/null || true
    cp "$BUILD_DIR/SIM_65C02.js" output/web/ 2>/dev/null || true
    cp "$BUILD_DIR/SIM_65C02.wasm" output/web/ 2>/dev/null || true
    cp "$BUILD_DIR/SIM_65C02.worker.js" output/web/ 2>/dev/null || true
    cp "src/Frontend/web/coi-serviceworker.js" output/web/ 2>/dev/null || true
    cp "src/Frontend/Assets/65c02-sim.svg" output/web/favicon.svg 2>/dev/null || true

    echo "Web build completed in output/web (index.html)"

    mkdir -p output/web/roms/midi
    mkdir -p output/web/roms/vram
    [ -d "output/cartridge" ] && cp output/cartridge/*.65c output/web/roms/ 2>/dev/null || true
    [ -d "output/rom" ] && cp output/rom/*.bin output/web/roms/ 2>/dev/null || true
    [ -d "output/midi" ] && cp output/midi/*.65c output/web/roms/midi/ 2>/dev/null || true
    [ -d "output/vram" ] && cp output/vram/*.65c output/web/roms/vram/ 2>/dev/null || true

    if command -v jq >/dev/null 2>&1; then
        echo "Generating structured roms.json..."
        ROMS_JSON=$(ls output/cartridge/*.65c 2>/dev/null | xargs -d '\n' -n 1 basename | jq -R . | jq -s . || echo "[]")
        MIDIS_JSON=$(ls output/midi/*.65c 2>/dev/null | xargs -d '\n' -n 1 basename | jq -R . | jq -s . || echo "[]")
        VRAMS_JSON=$(ls output/vram/*.65c 2>/dev/null | xargs -d '\n' -n 1 basename | jq -R . | jq -s . || echo "[]")
        jq -n --argjson r "$ROMS_JSON" --argjson m "$MIDIS_JSON" --argjson v "$VRAMS_JSON" \
            '{roms: $r, midis: $m, vrams: $v}' > output/web/roms/roms.json
    fi

    if [ "$DO_RUN" = true ]; then
        PORT=8080
        echo "Starting local server at http://localhost:$PORT..."
        if command -v xdg-open > /dev/null; then xdg-open "http://localhost:$PORT" &
        elif command -v open > /dev/null; then open "http://localhost:$PORT" & fi
        if command -v npx > /dev/null; then npx -y serve output/web -l $PORT
        else python3 -m http.server $PORT --directory output/web; fi
    fi

elif [[ "$TARGET_OS" == "win" ]]; then
    if [ ! -f "$BUILD_DIR/CMakeCache.txt" ] || [ "$FORCE_CONFIGURE" = true ]; then
        cmake -S . -B "$BUILD_DIR" -DCMAKE_TOOLCHAIN_FILE=cmake/mingw-toolchain.cmake -DCMAKE_BUILD_TYPE=$BUILD_TYPE $CMAKE_OPTS
    else
        echo "Skipping CMake configure (cache found). Use --reconfigure to force it."
    fi
    cmake --build "$BUILD_DIR" -j$(nproc)

    if command -v ccache >/dev/null 2>&1; then ccache -s; fi

    echo "Running unit tests (Windows/Wine)..."
    if command -v wine >/dev/null 2>&1; then
        wine "./$BUILD_DIR/unit_tests.exe"
    else
        echo "Wine not found, skipping unit tests."
    fi

    mkdir -p output
    cp "$BUILD_DIR/SIM_65C02.exe" output/ 2>/dev/null || true
    echo "Windows build completed in output"

elif [[ "$TARGET_OS" == "flatpak" ]]; then
    if [ "$LOCAL_SOURCES" = true ]; then
        MANIFEST_FILE="build/dev.manufloso.sim_65c02.ci.yml"
    else
        MANIFEST_FILE="flatpak/dev.manufloso.sim_65c02.yml"
    fi

    LOCAL_SOURCES="$LOCAL_SOURCES" MANIFEST_FILE="$MANIFEST_FILE" FORCE_CONFIGURE="$FORCE_CONFIGURE" DO_CLEAN="$DO_CLEAN" python3 - <<'PYEOF'
import datetime
import json
import os
import re
import shutil
import subprocess
import sys
import time
import urllib.request

REPO = "ManuFlosoYT/65c02-SIM"
API_BASE = f"https://api.github.com/repos/{REPO}"
METAINFO_PATH = "flatpak/dev.manufloso.sim_65c02.metainfo.xml"
MANIFEST_SRC_PATH = "flatpak/dev.manufloso.sim_65c02.yml"
MANIFEST_OUT_PATH = os.environ["MANIFEST_FILE"]
LOCAL_SOURCES = os.environ.get("LOCAL_SOURCES") == "true"
BYPASS_GH_CACHE = os.environ.get("FORCE_CONFIGURE") == "true" or os.environ.get("DO_CLEAN") == "true"
GH_CACHE_PATH = "build/.flatpak-gh-cache.json"
GH_CACHE_TTL_SEC = 600


def load_gh_cache():
    if BYPASS_GH_CACHE or not os.path.isfile(GH_CACHE_PATH):
        return {}
    try:
        with open(GH_CACHE_PATH, "r", encoding="utf-8") as f:
            return json.load(f)
    except (OSError, json.JSONDecodeError):
        return {}


def save_gh_cache(cache):
    try:
        os.makedirs(os.path.dirname(GH_CACHE_PATH), exist_ok=True)
        with open(GH_CACHE_PATH, "w", encoding="utf-8") as f:
            json.dump(cache, f, indent=2)
    except OSError:
        pass


def gh_headers():
    headers = {
        "User-Agent": "65c02-SIM-build",
        "Accept": "application/vnd.github+json",
    }
    token = os.environ.get("GITHUB_TOKEN") or os.environ.get("GH_TOKEN")
    if token:
        headers["Authorization"] = f"Bearer {token}"
    return headers


def gh_get(url):
    req = urllib.request.Request(url, headers=gh_headers())
    with urllib.request.urlopen(req, timeout=15) as resp:
        return json.loads(resp.read().decode("utf-8"))


def git_out(args):
    return subprocess.check_output(args, text=True, stderr=subprocess.DEVNULL).strip()


def parse_module_sources(lines, module_name):
    mod_idx = next(
        (i for i, l in enumerate(lines) if re.match(rf"^  - name:\s*{re.escape(module_name)}\b", l)),
        None,
    )
    if mod_idx is None:
        return []

    sources_idx = None
    for i in range(mod_idx + 1, len(lines)):
        if re.match(r"^  - name:", lines[i]):
            break
        if re.match(r"^    sources:\s*(?:#.*)?$", lines[i]):
            sources_idx = i
            break
    if sources_idx is None:
        return []

    sources_end = len(lines)
    for i in range(sources_idx + 1, len(lines)):
        stripped = lines[i].strip()
        if not stripped or stripped.startswith("#"):
            continue
        if len(lines[i]) - len(lines[i].lstrip(" ")) < 6:
            sources_end = i
            break

    starts = [i for i in range(sources_idx + 1, sources_end) if re.match(r"^      - ", lines[i])]
    items = []
    for idx, start in enumerate(starts):
        end = starts[idx + 1] if idx + 1 < len(starts) else sources_end
        while end > start + 1 and (lines[end - 1].strip() == "" or re.match(r"^ {0,6}#", lines[end - 1])):
            end -= 1
        items.append((start, end, "".join(lines[start:end])))
    return items


def update_metainfo(tag):
    version = tag.lstrip("v") or "0.0.0"
    today = datetime.date.today().isoformat()
    with open(METAINFO_PATH, "r", encoding="utf-8") as f:
        xml = f.read()

    new_releases = f'  <releases>\n    <release version="{version}" date="{today}"/>\n  </releases>'
    pattern = re.compile(r"[ \t]*<(?:releases>.*?</releases>|releases\s*/>)", re.DOTALL)
    if pattern.search(xml):
        updated = pattern.sub(lambda _m: new_releases, xml, count=1)
    else:
        updated = xml.replace("</component>", f"{new_releases}\n</component>", 1)

    with open(METAINFO_PATH, "w", encoding="utf-8") as f:
        f.write(updated)


def replace_sim_sources(yml_text, replacement_block):
    lines = yml_text.splitlines(keepends=True)
    items = parse_module_sources(lines, "sim_65c02")
    if not items:
        raise RuntimeError("Could not find 'sim_65c02' sources in Flatpak manifest")

    matched = [
        (s, e)
        for s, e, text in items
        if f"github.com/{REPO.lower()}" in text.lower()
        or (re.search(r"^\s*-\s*type:\s*dir\b", text, re.M) and re.search(r"^\s*path:\s*\.\.\s*$", text, re.M))
    ]
    if not matched:
        raise RuntimeError("Could not find 65c02-SIM or local dir source entries in 'sim_65c02'")

    first_start, last_end = matched[0][0], matched[-1][1]
    while first_start > 0 and re.match(r"^ {6}#", lines[first_start - 1]):
        first_start -= 1
    for s, e, text in items:
        if first_start <= s < last_end and (s, e) not in matched:
            raise RuntimeError(f"Unexpected source inside 65c02-SIM block:\n{text}")

    if not replacement_block.endswith("\n"):
        replacement_block += "\n"
    return "".join(lines[:first_start]) + replacement_block + "".join(lines[last_end:])


def resolve_from_github():
    cache = load_gh_cache()
    if (resolved := cache.get("resolved")) and (time.time() - resolved.get("ts", 0)) < GH_CACHE_TTL_SEC:
        return resolved["tag"], resolved["master_commit"]

    tags = gh_get(f"{API_BASE}/tags?per_page=1")
    if not tags:
        raise RuntimeError("No tags found on GitHub")

    tag = tags[0]["name"]
    master_commit = tags[0]["commit"]["sha"]

    cache = load_gh_cache()
    cache["resolved"] = {
        "ts": time.time(),
        "tag": tag,
        "master_commit": master_commit,
    }
    save_gh_cache(cache)

    return tag, master_commit


def resolve_offline():
    try:
        tag = git_out(["git", "describe", "--tags", "--abbrev=0"])
        master_commit = git_out(["git", "rev-list", "-n", "1", tag])
    except subprocess.CalledProcessError:
        tag = "v0.0.0"
        master_commit = git_out(["git", "rev-parse", "HEAD"])
    return tag, master_commit


with open(MANIFEST_SRC_PATH, "r", encoding="utf-8") as f:
    manifest_text = f.read()

if LOCAL_SOURCES:
    try:
        tag = git_out(["git", "describe", "--tags", "--abbrev=0"])
    except subprocess.CalledProcessError:
        tag = "v0.0.0"
    update_metainfo(tag)
    local_block = (
        "      - type: dir\n"
        "        path: ..\n"
        "        skip:\n"
        "          - build\n"
        "          - build_win\n"
        "          - emsdk_local\n"
        "          - .ccache\n"
        "          - output\n"
        "          - .flatpak-builder\n"
    )
    os.makedirs(os.path.dirname(MANIFEST_OUT_PATH), exist_ok=True)
    with open(MANIFEST_OUT_PATH, "w", encoding="utf-8") as f:
        f.write(replace_sim_sources(manifest_text, local_block))
    shutil.copyfile("flatpak/python3-modules.yml", os.path.join(os.path.dirname(MANIFEST_OUT_PATH), "python3-modules.yml"))
    print(f"Prepared local-sources manifest {MANIFEST_OUT_PATH} (metainfo version {tag.lstrip('v')})")
else:
    try:
        tag, master_c = resolve_from_github()
    except Exception as exc:
        print(f"Warning: GitHub API lookup failed ({exc}); falling back to local git.", file=sys.stderr)
        tag, master_c = resolve_offline()

    update_metainfo(tag)
    remote_block = (
        "      # Main emulator & IDE repository (master branch)\n"
        "      - type: git\n"
        f"        url: https://github.com/{REPO}.git\n"
        f"        tag: {tag}\n"
        f"        commit: {master_c}\n"
        "        disable-lfs: true\n"
    )
    updated_manifest = replace_sim_sources(manifest_text, remote_block)
    with open(MANIFEST_OUT_PATH, "w", encoding="utf-8") as f:
        f.write(updated_manifest)
    print(f"Updated {MANIFEST_OUT_PATH} and {METAINFO_PATH} for {tag} (master={master_c[:8]})")
PYEOF

    FLATPAK_BUILDER_ARGS=(
        --force-clean
        --delete-build-dirs
        --ccache
        --repo=build/flatpak-repo
        --compose-url-policy=full
        --mirror-screenshots-url=https://dl.flathub.org/media
    )
    if [ "$LOCAL_SOURCES" = true ] || [ ! -e /dev/fuse ]; then
        FLATPAK_BUILDER_ARGS+=(--disable-rofiles-fuse)
    fi

    rm -rf .flatpak-builder/build/*
    flatpak-builder "${FLATPAK_BUILDER_ARGS[@]}" build/flatpak-build "$MANIFEST_FILE"
    if [ -d build/flatpak-build/files/share/app-info/media ]; then
        ostree commit --repo=build/flatpak-repo --canonical-permissions --branch="screenshots/$(flatpak --default-arch)" build/flatpak-build/files/share/app-info/media
    fi
    mkdir -p output
    flatpak build-bundle build/flatpak-repo output/dev.manufloso.sim_65c02.flatpak dev.manufloso.sim_65c02
    echo "Flatpak bundle created at output/dev.manufloso.sim_65c02.flatpak"
    echo "Run with: flatpak install --user output/dev.manufloso.sim_65c02.flatpak && flatpak run dev.manufloso.sim_65c02"

    if command -v desktop-file-validate >/dev/null 2>&1; then
        desktop-file-validate flatpak/dev.manufloso.sim_65c02.desktop
    fi
    if command -v appstreamcli >/dev/null 2>&1; then
        appstreamcli validate --no-net flatpak/dev.manufloso.sim_65c02.metainfo.xml
    fi

    LINT_CMD=()
    if flatpak info org.flatpak.Builder >/dev/null 2>&1; then
        LINT_CMD=(flatpak run --command=flatpak-builder-lint org.flatpak.Builder)
    elif command -v flatpak-builder-lint >/dev/null 2>&1; then
        LINT_CMD=(flatpak-builder-lint)
    fi

    if [ ${#LINT_CMD[@]} -gt 0 ]; then
        if [ "$LOCAL_SOURCES" = false ]; then
            "${LINT_CMD[@]}" manifest "$MANIFEST_FILE"
        else
            echo "Skipping manifest lint in --local-sources mode ($MANIFEST_FILE uses local dir sources)."
        fi
        "${LINT_CMD[@]}" repo build/flatpak-repo
    else
        echo "Warning: flatpak-builder-lint not found. Install org.flatpak.Builder: flatpak install flathub org.flatpak.Builder" >&2
    fi

    if [ "$DO_RUN" = true ]; then
        flatpak install -y --user --reinstall output/dev.manufloso.sim_65c02.flatpak
        flatpak run dev.manufloso.sim_65c02
    fi

else
    # Linux build
    if [ ! -f "$BUILD_DIR/CMakeCache.txt" ] || [ "$FORCE_CONFIGURE" = true ]; then
        cmake -S . -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE=$BUILD_TYPE $CMAKE_OPTS
    else
        echo "Skipping CMake configure (cache found). Use --reconfigure to force it."
    fi
    cmake --build "$BUILD_DIR" -j$(nproc)

    if command -v ccache >/dev/null 2>&1; then ccache -s; fi

    echo "Running unit tests (Linux)..."
    ctest --test-dir "$BUILD_DIR" -j $(nproc) --output-on-failure

    mkdir -p output
    cp "$BUILD_DIR/SIM_65C02" output/ 2>/dev/null || true
    echo "Linux build completed in output"
fi

exit 0
