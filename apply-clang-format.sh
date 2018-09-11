#!/bin/bash

if [ "$1" == "--all" ]
then
    # run on all C++ source files
    SOURCE_FILES=`find -f include src test \( -name "*.h" -or -name "*.hpp" -or -name "*.hxx" -or -name "*.c" -or -name "*.cpp" -or -name "*.cxx" -or -name "*.cc" \)`
else
    # run only on changed/modified C++ source code 
    SOURCE_FILES=`git status --porcelain | awk '/^.*\.(h|hpp|hxx|c|cpp|cxx|cc)$/ {print $2}'`
fi
    
EXISTING_SOURCE_FILES=""
for FILE in ${SOURCE_FILES}
do
    if [ -e "${FILE}" ]
    then
        EXISTING_SOURCE_FILES+=" ${FILE}"
    fi
done

clang-format -i ${EXISTING_SOURCE_FILES}

