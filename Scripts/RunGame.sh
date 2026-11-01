#!/bin/bash
# RunGame.sh - launch the Runtime against a game project of your choosing.
#
# Usage:
#   ./Scripts/RunGame.sh                        Pick a project interactively
#   ./Scripts/RunGame.sh ExampleProject         Run that project directly
#   ./Scripts/RunGame.sh --config=Release       Pin the build config
#   ./Scripts/RunGame.sh --build                Build the Runtime first
#   ./Scripts/RunGame.sh --dry-run              Print what would launch, then stop
#
# With no project argument it lists every .lmproj in the tree and prompts.
# The last choice is remembered in .zed/.last-game-project and offered as the
# default, so repeat runs are just Enter.
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
LAST_FILE="$ROOT/.zed/.last-game-project"

CONFIG=""
PROJECT_ARG=""
DO_BUILD=false
DRY_RUN=false

for arg in "$@"; do
    case "$arg" in
        --config=*) CONFIG="${arg#--config=}" ;;
        --build|-b) DO_BUILD=true ;;
        --dry-run|-n) DRY_RUN=true ;;
        --help|-h)
            sed -n '2,13p' "${BASH_SOURCE[0]}" | sed 's/^# \{0,1\}//'
            exit 0
            ;;
        -*) echo "Unknown option: $arg" >&2; exit 1 ;;
        *)  PROJECT_ARG="$arg" ;;
    esac
done

# ---------------------------------------------------------------- pick project

# Resolves a user-supplied path (a .lmproj file or the directory holding one)
# to an absolute project directory. Prints nothing and fails if it isn't one.
resolve_project() {
    local input="${1/#\~/$HOME}"
    [ -z "$input" ] && return 1

    # Relative paths are resolved against the repo root, then the cwd.
    local candidate="$input"
    if [ ! -e "$candidate" ] && [ -e "$ROOT/$input" ]; then
        candidate="$ROOT/$input"
    fi
    [ -e "$candidate" ] || return 1

    if [ -f "$candidate" ]; then
        case "$candidate" in
            *.lmproj) candidate="$(dirname "$candidate")" ;;
            *) return 1 ;;
        esac
    fi

    [ -d "$candidate" ] || return 1
    compgen -G "$candidate/*.lmproj" > /dev/null || return 1
    (cd "$candidate" && pwd)
}

PROJECT_DIR=""

if [ -n "$PROJECT_ARG" ]; then
    if ! PROJECT_DIR="$(resolve_project "$PROJECT_ARG")"; then
        echo "Error: no .lmproj found at '$PROJECT_ARG'" >&2
        exit 1
    fi
else
    if [ ! -t 0 ]; then
        echo "Error: no project given and no terminal to prompt on." >&2
        echo "Usage: $0 [--config=Debug|Release|Production] <project-dir>" >&2
        exit 1
    fi

    mapfile -t PROJECTS < <(
        find "$ROOT" -name '*.lmproj' \
            -not -path '*/bin/*' -not -path '*/bin-int/*' -not -path '*/.git/*' \
            2>/dev/null | sort
    )

    DEFAULT=""
    if [ -f "$LAST_FILE" ]; then
        DEFAULT="$(<"$LAST_FILE")"
        [ -d "$DEFAULT" ] || DEFAULT=""
    fi

    echo "Lumos projects found:"
    echo
    for i in "${!PROJECTS[@]}"; do
        proj="${PROJECTS[$i]}"
        name="$(basename "$proj" .lmproj)"
        rel="${proj#"$ROOT"/}"
        marker=" "
        [ -n "$DEFAULT" ] && [ "$(dirname "$proj")" = "$DEFAULT" ] && marker="*"
        printf "  %s%2d) %-16s %s\n" "$marker" "$((i + 1))" "$name" "$(dirname "$rel")"
    done
    echo

    prompt="Select a project [1-${#PROJECTS[@]}] or type a path"
    if [ -n "$DEFAULT" ]; then
        prompt="$prompt (Enter for $(basename "$DEFAULT"))"
    fi

    while true; do
        read -r -e -p "$prompt: " choice

        if [ -z "$choice" ] && [ -n "$DEFAULT" ]; then
            PROJECT_DIR="$DEFAULT"
            break
        fi

        if [[ "$choice" =~ ^[0-9]+$ ]] && [ "$choice" -ge 1 ] && [ "$choice" -le "${#PROJECTS[@]}" ]; then
            PROJECT_DIR="$(dirname "${PROJECTS[$((choice - 1))]}")"
            break
        fi

        if PROJECT_DIR="$(resolve_project "$choice")"; then
            break
        fi

        echo "  Not a project directory - pick a number or a path containing a .lmproj."
    done

    mkdir -p "$(dirname "$LAST_FILE")"
    printf '%s' "$PROJECT_DIR" > "$LAST_FILE"
fi

# ---------------------------------------------------------------- find Runtime

if [ "$DO_BUILD" = true ]; then
    if [ "$(uname -s)" = "Darwin" ]; then
        # Xcode wants the capitalised config name; default matches BuildMacOS.sh.
        XC_CONFIG="${CONFIG:-Release}"
        echo "Building Runtime ($XC_CONFIG)..."
        ( cd "$ROOT" && xcodebuild -project Runtime/Runtime.xcodeproj \
            -parallelizeTargets -jobs 4 -configuration "$XC_CONFIG" \
            -sdk macosx -arch "$(uname -m)" \
            CODE_SIGN_IDENTITY="" CODE_SIGNING_REQUIRED=NO CODE_SIGNING_ALLOWED=NO )
    else
        # premake's make targets take the lowercase config name.
        MAKE_CONFIG="$(echo "${CONFIG:-Debug}" | tr '[:upper:]' '[:lower:]')"
        echo "Building Runtime ($MAKE_CONFIG)..."
        ( cd "$ROOT" && make "config=$MAKE_CONFIG" Runtime -j"$(nproc)" )
    fi
fi

# Collect candidate binaries: plain executables on Linux/Windows, and the
# executable inside the .app bundle on macOS.
mapfile -t CANDIDATES < <(
    {
        find "$ROOT/bin" -mindepth 2 -maxdepth 2 -name 'Runtime' -type f -perm -u+x 2>/dev/null
        find "$ROOT/bin" -path '*/Runtime.app/Contents/MacOS/Runtime' -type f -perm -u+x 2>/dev/null
    } | sort
)

if [ -n "$CONFIG" ]; then
    mapfile -t CANDIDATES < <(printf '%s\n' "${CANDIDATES[@]}" | grep "/bin/${CONFIG}-" || true)
fi

if [ "${#CANDIDATES[@]}" -eq 0 ] || [ -z "${CANDIDATES[0]}" ]; then
    echo "Error: no Runtime binary found under $ROOT/bin${CONFIG:+ for config '$CONFIG'}" >&2
    echo "Build one first, e.g.  make config=debug Runtime -j\$(nproc)" >&2
    exit 1
fi

# Prefer whatever was built most recently.
RUNTIME=""
for bin in "${CANDIDATES[@]}"; do
    if [ -z "$RUNTIME" ] || [ "$bin" -nt "$RUNTIME" ]; then
        RUNTIME="$bin"
    fi
done

# ---------------------------------------------------------------------- launch

echo
echo "Project : $PROJECT_DIR"
echo "Runtime : ${RUNTIME#"$ROOT"/}"
echo

cd "$(dirname "$RUNTIME")"

if [ "$DRY_RUN" = true ]; then
    echo "[dry run] cd $(pwd) && ./$(basename "$RUNTIME") --project=$PROJECT_DIR"
    exit 0
fi

exec "$RUNTIME" --project="$PROJECT_DIR"
