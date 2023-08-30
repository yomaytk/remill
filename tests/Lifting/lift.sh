#!/usr/bin/env bash

SOURCE_DIR=${HOME}/workspace/compiler/remill
BUILD_DIR=${SOURCE_DIR}/build3
BUILD_LIFTING_DIR=${BUILD_DIR}/tests/Lifting

cd $BUILD_DIR && \
    ./tests/Lifting/lifting_target_aarch64 \
    --arch aarch64 \
    --bc_out ${BUILD_LIFTING_DIR}/lifting_target_aarch64.bc && \
    llvm-dis ${BUILD_LIFTING_DIR}/lifting_target_aarch64.bc -o ${BUILD_LIFTING_DIR}/lifting_target-aarch64.ll
