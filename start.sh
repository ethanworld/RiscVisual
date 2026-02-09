#!/bin/bash
current_path=$PWD
rela_path="elf/a.out"
full_path=$(readlink -f "$current_path/$rela_path")
echo "ELF文件路径: $full_path"

gcc -O0 readelf.c -g
if [ $? -eq 0 ]; then
    echo "success"
else
    echo "failed"
    return 1
fi
./a.out $full_path
if [ $? -eq 0 ]; then
    echo "success"
else
    echo "failed"
    return 1
fi