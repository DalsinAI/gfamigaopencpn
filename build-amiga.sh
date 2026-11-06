#!/bin/sh
# Builds OpenCPN 5.14.2 for AmigaOS 3: 68040, the FPU when there is one
# (fpuauto), charts drawn with wxDC (OpenGL off). The GFOpenWX Toolkit's SDK
# first (once for each toolkit commit), fpuauto's library, then OpenCPN.
#   STOVE=.../stoves/os32-gcc16-open6 OUT=out ./build-amiga.sh
# OpenCPN, shapelib, SQLite (as gfopensqlite builds it),
# json.library, gfopencrypto.library, OXP (gfopenxml), the toolkit and fpuauto are fetched at the commits opencpn.lock gives; nothing
# third-party is kept here. OUT gets
# OpenCPN and OpenCPN-<version>.lha (the program with its data drawer).
# Copyright (c) 2026 AmiCradle Team
set -eu
HERE=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
. "$HERE/opencpn.lock"
STOVE=${STOVE:-"$HOME/AmiCradle/stoves/os32-gcc16-open6"}
# the work tree lives outside the checkout (Cradle cleans its checkout
# before every job), so a run that stops at the time limit goes on next time
WORK="${WORK:-${XDG_CACHE_HOME:-$HOME/.cache}/gfamigaopencpn/build}"
OUT="${OUT:-$WORK/out}"; JOBS=${JOBS:-12}
TC="$STOVE/prefix"
mkdir -p "$WORK/deps" "$OUT"
t0=$(date +%s)
echo "stove: $("$TC/bin/m68k-amigaos-gcc" --version | head -1)"

fetch() {   # name repo commit [paths...]: one commit, its tree (or those paths)
    d="$WORK/deps/$1"; repo=$2; c=$3; shift 3
    if [ "$(cat "$d/.commit" 2>/dev/null)" != "$c" ]; then
        rm -rf "${d:?}"; mkdir -p "$d"
        git -C "$d" init -q
        if [ $# -gt 0 ]; then   # only those paths' files are downloaded
            git -C "$d" fetch -q --depth 1 --filter=blob:none "$repo" "$c"
            git -C "$d" checkout -q FETCH_HEAD -- "$@"
        else
            git -C "$d" fetch -q --depth 1 "$repo" "$c"; git -C "$d" checkout -q FETCH_HEAD
        fi
        echo "$c" > "$d/.commit"
    fi
}

# ---- the toolkit's SDK, when its commit changed
fetch gfopenwxtoolkit "$WXT_REPO" "$WXT_COMMIT"
WXT="$WORK/deps/gfopenwxtoolkit"; WXB="$WORK/gfopenwx"
if [ ! -x "$WXB/out/sdk/bin/wx-config" ] || [ "$(cat "$WXB/.rev" 2>/dev/null)" != "$WXT_COMMIT" ]; then
    echo "toolkit $WXT_COMMIT: building its SDK"
    # under Cradle's 900 s a long SDK build is stopped here first, so the job
    # says how far it got; its trees are kept and the next run goes on
    WXT_BUDGET=${WXT_BUDGET:-780}
    touch "$WORK/.wxstart"
    rc=0; timeout "$WXT_BUDGET" env STOVE="$STOVE" WORK="$WXB" OUT="$WXB/out" JOBS=$JOBS \
        sh "$WXT/build-amiga.sh" > "$WORK/wx.log" 2>&1 || rc=$?
    if [ "$rc" -ne 0 ]; then
        [ "$rc" -eq 124 ] && echo "toolkit SDK: not done after $WXT_BUDGET s; run again to go on"
        echo "toolkit objects: base $(find "$WXB/base" -name '*.o' 2>/dev/null | wc -l), gui $(find "$WXB" -path "$WXB/base" -prune -o -name '*.o' -print 2>/dev/null | wc -l)"
        echo "toolkit objects made this run: $(find "$WXB/gui/obj" -name '*.o' -newer "$WORK/.wxstart" 2>/dev/null | wc -l)"
        echo "toolkit gui objects, newest:"; ls -lt "$WXB/gui/obj" 2>/dev/null | head -8
        echo "toolkit gui objects, oldest:"; ls -ltr "$WXB/gui/obj" 2>/dev/null | grep '\.o$' | head -3
        ls -l "$WXB/gui"/*.a 2>/dev/null
        tail -12 "$WORK/wx.log"; exit 1
    fi
    echo "$WXT_COMMIT" > "$WXB/.rev"
    echo "toolkit: $(( $(date +%s) - t0 )) s"
fi
[ -x "$WXB/out/sdk/bin/wx-config" ] || { tail -40 "$WORK/wx.log"; echo "no toolkit SDK"; exit 1; }
SDK="$WXB/out/sdk"

# ---- fpuauto: libfpuauto.a from its fpuauto.S, as its build-amiga.sh makes it
fetch fpuauto "$FPUAUTO_REPO" "$FPUAUTO_COMMIT" common/fpuauto
FA="$WORK/deps/fpuauto/common/fpuauto"; FAL="$WORK/fpuauto"
if [ "$(cat "$FAL/.commit" 2>/dev/null)" != "$FPUAUTO_COMMIT" ]; then
    rm -rf "$FAL"; mkdir -p "$FAL/obj"
    "$TC/bin/m68k-amigaos-gcc" -m68040 -DFPUAUTO_COMMON -c "$FA/fpuauto.S" -o "$FAL/obj/fa_common.o"
    for n in $(sed 's/^--wrap=//' "$FA/fpuauto.wraps"); do
        "$TC/bin/m68k-amigaos-gcc" -m68040 -DR_$n -c "$FA/fpuauto.S" -o "$FAL/obj/fa_$n.o"
    done
    "$TC/bin/m68k-amigaos-ar" rcs "$FAL/libfpuauto.a" "$FAL"/obj/fa_*.o
    echo "$FPUAUTO_COMMIT" > "$FAL/.commit"
fi

# ---- SQLite as gfopensqlite builds it: its amalgamation (checked against
# the SHA-256 in opencpn.lock) with its AmigaDOS patch; its compat/ is
# compiled with OpenCPN (scripts/Makefile.amiga)
fetch gfopensqlite "$GFSQLITE_REPO" "$GFSQLITE_COMMIT"
GFSQL="$WORK/deps/gfopensqlite"; SQL="$WORK/sqlite"
if [ "$(cat "$SQL/.stamp" 2>/dev/null)" != "$GFSQLITE_COMMIT $SQLITE_SHA256" ]; then
    Z="$WORK/deps/$SQLITE_ZIP.zip"
    [ -f "$Z" ] || curl -sSfL -o "$Z" "$SQLITE_URL"
    echo "$SQLITE_SHA256  $Z" | sha256sum -c - >/dev/null || { echo "SHA-256 mismatch: $Z"; rm -f "$Z"; exit 1; }
    rm -rf "${SQL:?}"; mkdir -p "$SQL"
    (cd "$WORK/deps" && rm -rf "$SQLITE_ZIP" && unzip -q "$Z")
    cp "$WORK/deps/$SQLITE_ZIP"/sqlite3.c "$WORK/deps/$SQLITE_ZIP"/sqlite3.h "$WORK/deps/$SQLITE_ZIP"/sqlite3ext.h "$SQL"/
    patch -s -d "$SQL" -p1 < "$GFSQL/patches/sqlite-3.53.4-amiga-paths.patch"
    echo "$GFSQLITE_COMMIT $SQLITE_SHA256" > "$SQL/.stamp"
fi

# ---- json.library (GFOpenJSON), with GFOpenBrowser's own build script;
# OpenCPN's JSON is read and written by it (src/wxjson_jsonlib.cpp)
fetch gfjson "$GFJSON_REPO" "$GFJSON_COMMIT" src/jsonlib scripts/build-json-library.sh toolchain/m68k-soft-m68040.specs
GFJSON="$WORK/deps/gfjson"; JL="$WORK/jsonlib"
if [ ! -f "$JL/json.library" ] || [ "$(cat "$JL/.commit" 2>/dev/null)" != "$GFJSON_COMMIT" ]; then
    rm -rf "${JL:?}"
    STOVE="$STOVE" sh "$GFJSON/scripts/build-json-library.sh" "$JL" > "$WORK/jsonlib.log" 2>&1 \
        || { tail -20 "$WORK/jsonlib.log"; exit 1; }
    echo "$GFJSON_COMMIT" > "$JL/.commit"
fi

# ---- gfopencrypto.library (GFOpenCrypto), with its own build script; OpenCPN's
# SHA-1 runs on it (src/sha1_gfcrypto.c)
fetch gfcrypto "$GFCRYPTO_REPO" "$GFCRYPTO_COMMIT" build-amiga.sh specs src library include tests
GFCRYPTO="$WORK/deps/gfcrypto"; CL="$WORK/gfcrypto"
if [ ! -f "$CL/lib/gfopencrypto.library" ] || [ "$(cat "$CL/.commit" 2>/dev/null)" != "$GFCRYPTO_COMMIT" ]; then
    rm -rf "${CL:?}"
    STOVE="$STOVE" OUT="$CL" sh "$GFCRYPTO/build-amiga.sh" > "$WORK/gfcrypto.log" 2>&1 \
        || { tail -20 "$WORK/gfcrypto.log"; exit 1; }
    echo "$GFCRYPTO_COMMIT" > "$CL/.commit"
fi

# ---- OXP, GFOpenXML's parser: OpenCPN's XML (src/pugixml_oxp.cpp, src/tinyxml_oxp.cpp)
fetch gfxml "$GFXML_REPO" "$GFXML_COMMIT" oxp/include oxp/src/oxp.c oxp/src/oxp_dom.c oxp/src/oxp_int.h
GFXML="$WORK/deps/gfxml"

# ---- shapelib, with the patches OpenCPN applies to it
fetch shapelib "$SHAPELIB_REPO" "$SHAPELIB_COMMIT"
fetch opencpn "$OCPN_REPO" "$OCPN_COMMIT"
OC="$WORK/deps/opencpn"

# ---- OpenCPN's source, with its library patches and ours (again when they
# change). The tree is made afresh and copied over the last one by content,
# so only files a patch changed get new times and make rebuilds only those
# (the makefile keeps each object's header list)
SRC="$WORK/opencpn-$OCPN_VERSION"; SHP="$WORK/shapelib"
PSUM=$(cat "$HERE"/patches/*.patch "$HERE/opencpn.lock" | sha256sum | cut -c1-16)
if [ "$(cat "$SRC/.patched" 2>/dev/null)" != "$PSUM" ]; then
    F="$WORK/fresh"; rm -rf "$F"; mkdir -p "$F/opencpn" "$F/shapelib"
    cp -R "$OC"/. "$F/opencpn"/; rm -rf "$F/opencpn/.git"
    cp -R "$WORK/deps/shapelib"/. "$F/shapelib"/
    rm -rf "$F/shapelib/.git"
    for p in "$F"/opencpn/libs/shapelib/patches/*.patch; do patch -s -d "$F/shapelib" -p1 --ignore-whitespace < "$p"; done
    for p in "$F"/opencpn/libs/ShapefileCpp/patches/*.patch; do
        patch -s -d "$F/opencpn/libs/ShapefileCpp/ShapeFileCpp-b2681f8" -p1 --ignore-whitespace < "$p"
    done
    for p in "$HERE"/patches/*.patch; do
        patch -s -d "$F/opencpn" -p1 < "$p" || { echo "patch failed: $p"; exit 1; }
    done
    mkdir -p "$SRC" "$SHP"
    if command -v rsync >/dev/null 2>&1; then
        rsync -rc --delete "$F/opencpn/" "$SRC/"
        rsync -rc --delete "$F/shapelib/" "$SHP/"
    else
        rm -rf "$SRC" "$SHP" "$WORK/obj"; mv "$F/opencpn" "$SRC"; mv "$F/shapelib" "$SHP"
    fi
    rm -rf "$F"
    echo "$PSUM" > "$SRC/.patched"
fi

# ---- OpenCPN
echo "opencpn: building (from $(( $(date +%s) - t0 )) s)"
nice -n 19 make -k -j"$JOBS" -f "$HERE/scripts/Makefile.amiga" SRC="$SRC" SHP="$SHP" SQL="$SQL" GFSQL="$GFSQL" GFJSON="$GFJSON" GFXML="$GFXML" GFCRYPTO="$GFCRYPTO" \
    OBJ="$WORK/obj" OUT="$OUT" TC="$TC" WXCONFIG="$SDK/bin/wx-config" PORT="$HERE" \
    SPECS="$HERE/scripts/m68k-soft-m68040.specs" FPUSPECS="$FA/fpuauto.specs" FPULIB="$FAL" \
    > "$WORK/opencpn.log" 2>&1 || {
        echo "objects: $(find "$WORK/obj" -name '*.o' | wc -l)"
        # each error once, without make's own lines, the first 150
        grep -E 'error:|undefined reference' "$WORK/opencpn.log" | sed "s#$SRC/##g; s#$WORK/##g" \
            | cut -c1-240 | awk '!seen[$0]++' | head -150
        tail -5 "$WORK/opencpn.log" | cut -c1-240; }
[ -f "$OUT/OpenCPN" ] || exit 1
ls -l "$OUT/OpenCPN"
OD="$TC/bin/m68k-amigaos-objdump"
echo "fpuauto: $("$OD" -d "$OUT/OpenCPN" | grep -c 'jsr.*<___wrap_' || true) wrapped calls"

# ---- the package: OpenCPN's drawer, with the data it reads at start
P="$WORK/pkg/OpenCPN"
rm -rf "$WORK/pkg"; mkdir -p "$P"
cp "$OUT/OpenCPN" "$P/"
mkdir -p "$P/Libs"; cp "$JL/json.library" "$CL/lib/gfopencrypto.library" "$P/Libs/"
cp -R "$SRC/data" "$P/"
mkdir -p "$P/uidata"; cp -R "$SRC/resources"/. "$P/uidata/" 2>/dev/null || :
cp "$SRC/COPYING.gplv2" "$P/" 2>/dev/null || :
if command -v lha >/dev/null 2>&1; then
    (cd "$WORK/pkg" && lha -aq "$OUT/OpenCPN-$OCPN_VERSION.lha" OpenCPN >/dev/null) && ls -l "$OUT/OpenCPN-$OCPN_VERSION.lha"
elif command -v jlha >/dev/null 2>&1; then
    (cd "$WORK/pkg" && jlha -aq "$OUT/OpenCPN-$OCPN_VERSION.lha" OpenCPN >/dev/null) && ls -l "$OUT/OpenCPN-$OCPN_VERSION.lha"
else
    (cd "$WORK/pkg" && tar cf "$OUT/OpenCPN-$OCPN_VERSION.tar" OpenCPN) && echo "no lha here: $OUT/OpenCPN-$OCPN_VERSION.tar"
fi
echo "total: $(( $(date +%s) - t0 )) s"
