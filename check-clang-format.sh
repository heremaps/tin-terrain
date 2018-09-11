#!/bin/bash

if [ "$1" == "--all" ]
then
    HAS_ALL=1
    # run on all C++ source files
    SOURCE_FILES=`find -f include src test \( -name "*.h" -or -name "*.hpp" -or -name "*.hxx" -or -name "*.c" -or -name "*.cpp" -or -name "*.cxx" -or -name "*.cc" \)`
else
    HAS_ALL=0
    # run only on changed/modified C++ source code 
    SOURCE_FILES=`git status --porcelain | awk '/^.*\.(h|hpp|hxx|c|cpp|cxx|cc)$/ {print $2}'`
fi

FILES_TO_FIX_COUNT=0
for FILE in ${SOURCE_FILES}
do
    DIFF=`clang-format "${FILE}" | diff -u "${FILE}" -`
    if [ -n "${DIFF}" ] ; then
        echo "${DIFF}"
        FILES_TO_FIX_COUNT=$((FILES_TO_FIX_COUNT+1))
    fi
done


if [ "${FILES_TO_FIX_COUNT}" -gt 0 ]; then
    echo "some files need style fixes"
    exit -1
else
    if [ "${HAS_ALL}" -eq 1 ]; then
        echo "all files are ok"
    else
        echo "changed files are ok"
    fi
    
    exit 0
fi
