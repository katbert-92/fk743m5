#!/usr/bin/env bash

# chmod +x utils/code-format.sh
# utils/code-format.sh

INCLUDE_DIRS=("app" "bsp" "lib" "platform")
EXCLUDE_DIRS=("lib/fatfs" "lib/rtos" "platform/m0/core" "platform/m0/cube_mx" "platform/m0/usb_device")

PROJECT_ROOT=$(dirname "$(dirname "$0")")

# format_files() {
#     changed_files=$(git diff --name-only dev...HEAD)

#     for file in $changed_files; do
#         skip=false
        
#         for exclude_dir in "${EXCLUDE_DIRS[@]}"; do
#             if [[ $file == $exclude_dir/* ]]; then
#                 skip=true
#                 break
#             fi
#         done

#         if [ "$skip" = false ]; then
#             for dir in "${INCLUDE_DIRS[@]}"; do
#                 if [[ $file == $dir/* ]]; then
#                     clang-format -i "$PROJECT_ROOT/$file"
#                     echo "Formatted: $PROJECT_ROOT/$file"
#                     break 
#                 fi
#             done
#         fi
#     done
# }

format_files() {
    for dir in "${INCLUDE_DIRS[@]}"; do
        if [ -d "$PROJECT_ROOT/$dir" ]; then
            find "$PROJECT_ROOT/$dir" -name '*.c' -o -name '*.h' | while read -r file; do
                skip=false
                for excluded in "${EXCLUDE_DIRS[@]}"; do
                    if [[ $file == "$PROJECT_ROOT/$excluded"* ]]; then
                        skip=true
                        break
                    fi
                done

                if [ "$skip" = false ]; then
                    clang-format -i "$file"
                    echo "Formatted: $file"
                fi
            done
        else
            echo "Directory $PROJECT_ROOT/$dir does not exist."
        fi
    done
}

format_files