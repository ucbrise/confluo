#!/usr/bin/env bash

sbin="`dirname "$0"`"
sbin="`cd "$sbin"; pwd`"

# Genrate cpp files
thrift -r --gen cpp:templates,pure_enums,no_skeleton,include_prefix $sbin/rpc.thrift
# Modify cpp extensions
cpp_files=gen-cpp/*.cpp
for file in $cpp_files; do
    mv "$file" ${file/%.cpp/.cc}
done

# Move to destination
mv gen-cpp/*.cc $sbin/../librpc/src/
mv gen-cpp/* $sbin/../librpc/rpc/

# Generate java files
thrift -r --gen java:private-members,fullcamel,reuse-objects $sbin/rpc.thrift

# Move to destination
rsync -r --remove-source-files  gen-java/confluo $sbin/../javaclient/src/main/java

# Generate python files
thrift -r --gen py $sbin/rpc.thrift

# Move to destination
rsync -r --remove-source-files gen-py/confluo $sbin/../pyclient/
