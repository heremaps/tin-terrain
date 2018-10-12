#!/bin/bash

git ls-files | grep -Ee "\.(h|.pp)$" \
 | xargs -I{} clang-format -i -style=file {}

dirty=$(git ls-files --modified | grep -Ee "\.(h|.pp)$")

if [[ $dirty ]]; then
    echo "The following files have been modified:"
    echo $dirty
    exit 1
else
    echo "All good"
    exit 0
fi