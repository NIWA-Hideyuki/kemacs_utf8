#!/bin/sh
# Regenerate kemacs .c/.h files from .C_/.H_ templates using kpp
# This script must be run from the kemacs source directory

set -e

# Start fresh: delete any stale string table data
rm -f Cstr.data Cxx.c

# Process .H_ header files first (so .h files exist for .C_ includes)
# Then process .C_ source files
# kpp accumulates strings into Cstr.data across all invocations

echo "=== Processing .H_ header files ==="
for f in *.H_; do
    out=$(echo "$f" | sed -e 's/\.H_$/.h/')
    echo "  kpp $f -> $out"
    ./kpp/kpp "$f"
    cp Cxx.c "$out"
done

echo "=== Processing .C_ source files ==="
for f in *.C_; do
    out=$(echo "$f" | sed -e 's/\.C_$/.c/')
    echo "  kpp $f -> $out"
    ./kpp/kpp "$f"
    cp Cxx.c "$out"
done

echo "=== Generating string table (Css.c) from Cstr.data ==="
./kpp/ftoc

echo "=== Done ==="
ls -la Cstr.data Css.c
