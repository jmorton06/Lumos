#!/bin/sh
echo "Compiling shaders"
cd "$(dirname "$0")"

COMPILER="/home/jmorton/Dev/1.4.313.0/x86_64/bin/glslc"
echo $COMPILER

DSTDIR=CompiledSPV

FORCE_RECOMPILE=0
SHADER_FILTER=""

while [[ "$#" -gt 0 ]]; do
    case $1 in
        -f|--force) FORCE_RECOMPILE=1 ;;
        -s|--shader) SHADER_FILTER="$2"; shift ;;
        *) echo "Unknown parameter passed: $1"; exit 1 ;;
    esac
    shift
done

for SRC in *.vert *.frag *.comp; do

    if [ -e $SRC ]
    then
        # Check if shader filter is set and if the file matches
        if [ -n "$SHADER_FILTER" ] && [[ "$SRC" != *"$SHADER_FILTER"* ]]; then
            continue
        fi

        OUT="CompiledSPV/$(echo "$SRC" | sed "s/\.frag$/.frag/" | sed "s/\.vert$/.vert/" | sed "s/\.comp$/.comp/").spv"

        if [ -e $OUT ] && [ "$FORCE_RECOMPILE" -eq 0 ]
        then
            # if output exists
            # don't re-compile if existing binary is newer than source file
            NEWER="$(ls -t1 "$SRC" "$OUT" | head -1)"

            if [ "$SRC" = "$NEWER" ]; then

                echo "Compiling $OUT from:"

                $COMPILER  "$SRC" -o "$OUT"
            #else
               # echo "(Unchanged $SRC)"
            fi
        else
            echo "Compiling $OUT from:"
            $COMPILER "$SRC" -o "$OUT"
        fi
    fi
done

echo "Finished Compiling Shaders"
