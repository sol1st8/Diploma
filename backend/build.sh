#!/bin/bash

export DB_URL=export DB_URL=postgresql://postgres:4317321@localhost:30432/central

mkdir -p build && cd build

conan install .. --build=missing -s build_type=Release -s compiler.libcxx=libstdc++11

cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build .
