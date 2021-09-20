#!/bin/bash

mkdir -p cmake-build-debug && \
  cd ./cmake-build-debug && \
  cmake .. &&  cmake --build . --target all -- -j 9
