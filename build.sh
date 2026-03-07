#!/bin/bash

for dir in */ ; do
    dir_name="${dir%/}"
    cd "$dir_name"

    ./build_opk.sh

    cd ..

done
