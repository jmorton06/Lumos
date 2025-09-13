#!/bin/sh
echo "Compiling shaders"
cd "$(dirname "$0")"

COMPILER="/Users/jmorton/VulkanSDK/1.4.321.0/macOS/bin/glslc"

echo $COMPILER

DSTDIR=CompiledSPV

for SRC in *.vert *.frag *.comp; do

    if [ -e $SRC ]
    then
        OUT="CompiledSPV/$(echo "$SRC" | sed "s/\.frag$/.frag/" | sed "s/\.vert$/.vert/" | sed "s/\.comp$/.comp/").spv"

        if [ -e $OUT ]
        then
            # if output exists
            # don't re-compile if existing binary is newer than source file
            NEWER="$(ls -t1 "$SRC" "$OUT" | head -1)"

            if [ "$SRC" = "$NEWER" ]; then

                echo "Compiling $OUT from: $SRC"

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
