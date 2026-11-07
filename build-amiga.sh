#!/bin/sh
# For the Cradle's stove_build (STOVE= and OUT= set, target .): build.sh into OUT.
# The sources it needs beside it are taken from the Cradle's own clean
# checkouts (cradle-builds/DalsinAI__<repo>) when they are there, else from
# beside this checkout (OGT= and the others to say where).
# MIT, Copyright (c) 2026 AmiCradle Team.
set -eu
HERE=$(cd "$(dirname "$0")" && pwd)
up=$(dirname "$HERE")
TMP=$(mktemp -d); trap 'rm -rf "$TMP"' EXIT
# A Cradle checkout can be a git directory with no files out: take its tree.
tree() {
    [ -n "$(ls "$1" 2>/dev/null)" ] && { echo "$1"; return; }
    for r in HEAD FETCH_HEAD origin/main main; do
        if git -C "$1" rev-parse -q --verify "$r^{tree}" >/dev/null 2>&1; then
            mkdir -p "$TMP/$2" && git -C "$1" archive "$r" | tar -x -C "$TMP/$2" && { echo "$TMP/$2"; return; }
        fi
    done
    echo "$1"
}
# pick REPO: a checkout with files beside this one, else DalsinAI/REPO's main fetched from GitHub
# (a Cradle job checks out this repository alone).
pick() {
    if [ -d "$up/DalsinAI__$1" ]; then d=$(tree "$up/DalsinAI__$1" "$1"); [ -n "$(ls "$d" 2>/dev/null)" ] && { echo "$d"; return; }; fi
    [ -n "$(ls "$up/$1" 2>/dev/null)" ] && { echo "$up/$1"; return; }
    git clone -q --depth 1 "${AMICRADLE_GIT_BASE:-https://github.com/}DalsinAI/$1" "$TMP/git-$1" >&2 &&
        { echo "build-amiga.sh: $1 at $(git -C "$TMP/git-$1" rev-parse --short HEAD) (main)" >&2; echo "$TMP/git-$1"; return; }
    echo "$up/$1"
}
OUT=${OUT:-$HERE/build/os3}
OS32_GCC16=${OS32_GCC16:-${STOVE:-$HOME/AmiCradle/stoves/os32-gcc16}}
export OS32_GCC16
OGT=${OGT:-$(pick gfopengadtools)}
export OGT
sh "$HERE/build.sh" "$OUT"
