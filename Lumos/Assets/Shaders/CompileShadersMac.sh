#!/bin/sh
echo "Compiling shaders"
cd "$(dirname "$0")"

COMPILER="/Users/jmorton/VulkanSDK/1.4.321.0/macOS/bin/glslc"

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

                echo "Compiling $OUT from: $SRC"

                $COMPILER "$SRC" -o "$OUT"
            #else
               # echo "(Unchanged $SRC)"
            fi
        else
            echo "Compiling $OUT from: $SRC"
            $COMPILER "$SRC" -o "$OUT"
        fi
    fi
done

# Compile QUALITY_LOW variant of ForwardPBR fragment shader
LOWSRC="ForwardPBR.frag"
LOWOUT="CompiledSPV/ForwardPBR_Low.frag.spv"
if [ -e "$LOWSRC" ]; then
    if [ ! -e "$LOWOUT" ] || [ "$FORCE_RECOMPILE" -eq 1 ] || [ "$LOWSRC" = "$(ls -t1 "$LOWSRC" "$LOWOUT" 2>/dev/null | head -1)" ]; then
        echo "Compiling $LOWOUT from: $LOWSRC (QUALITY_LOW)"
        $COMPILER -DQUALITY_LOW=1 "$LOWSRC" -o "$LOWOUT"
    fi
fi

# Compile FORWARD_PLUS variant of ForwardPBR fragment shader
PLUSSRC="ForwardPBR.frag"
PLUSOUT="CompiledSPV/ForwardPBRPlus.frag.spv"
if [ -e "$PLUSSRC" ]; then
    if [ ! -e "$PLUSOUT" ] || [ "$FORCE_RECOMPILE" -eq 1 ] || [ "$PLUSSRC" = "$(ls -t1 "$PLUSSRC" "$PLUSOUT" 2>/dev/null | head -1)" ]; then
        echo "Compiling $PLUSOUT from: $PLUSSRC (FORWARD_PLUS)"
        $COMPILER -DFORWARD_PLUS=1 "$PLUSSRC" -o "$PLUSOUT"
    fi
fi

echo "Finished Compiling Shaders"
