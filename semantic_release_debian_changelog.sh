#!/bin/bash

export DEBFULLNAME="Rick Bassham"
export DEBEMAIL="brodrick.bassham@gmail.com"

dch --newversion $1 --empty --distribution focal "new release $1"

while IFS= read -r line; do
    if [[ $line == "*"* ]]; then
        dch -a --nomultimaint `echo "$line" | sed 's/* //' | sed 's/([^)]*)//' | sed 's/ )//'`
    fi
done <<< "$2"

IFS='.' read -r -a version <<< "$1"

sed -i 's/set(CCGX_VERSION_MAJOR [[:digit:]]\+)/set(CCGX_VERSION_MAJOR '"${version[0]}"')/' ./CMakeLists.txt
sed -i 's/set(CCGX_VERSION_MINOR [[:digit:]]\+)/set(CCGX_VERSION_MINOR '"${version[1]}"')/' ./CMakeLists.txt
