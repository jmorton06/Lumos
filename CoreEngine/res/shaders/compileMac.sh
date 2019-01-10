#!/bin/sh
echo "Compiling shaders"

cd /Users/josephmorton/dev/UnititledEngine/CoreEngine/res/shaders

compiler="/Users/josephmorton/vulkansdk-macos-1.1.85.0/macOS/bin/glslangValidator"

echo $compiler

for file in ./*
do
    if [[ ${file: -5} = ".frag" || ${file: -5} = ".vert" ]]
    then
        echo "Compiling glsl file: $file"
        $compiler -V -o "$file.spv" $file
    fi
done
