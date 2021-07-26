#!/bin/bash

THIS_PATH="$(realpath "$0")"
ENGINE_PATH="$(dirname "$THIS_PATH")/../Lumos/Source"
Runtime_PATH="$(dirname "$THIS_PATH")/../Editor/Source"
EDITOR_PATH="$(dirname "$THIS_PATH")/../Runtime"

FILE_LIST="$(find "$ENGINE_PATH" | grep -E ".*(\.ino|\.cpp|\.c|\.h|\.hpp|\.hh)$")"

echo -e "Files found to format = \n\"\"\"\n$FILE_LIST\n\"\"\""

# Format each file.
clang-format --verbose -i --style=file $FILE_LIST

FILE_LIST="$(find "$Runtime_PATH" | grep -E ".*(\.ino|\.cpp|\.c|\.h|\.hpp|\.hh)$")"

echo -e "Files found to format = \n\"\"\"\n$FILE_LIST\n\"\"\""

# Format each file.
clang-format --verbose -i --style=file $FILE_LIST

FILE_LIST="$(find "$EDITOR_PATH" | grep -E ".*(\.ino|\.cpp|\.c|\.h|\.hpp|\.hh)$")"

echo -e "Files found to format = \n\"\"\"\n$FILE_LIST\n\"\"\""

# Format each file.
clang-format --verbose -i --style=file $FILE_LIST