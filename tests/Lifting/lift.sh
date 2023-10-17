#!/usr/bin/env bash

SOURCE_DIR=${HOME}/workspace/compiler/remill
SOURCE_LIFTING_DIR=${SOURCE_DIR}/tests/Lifting
BUILD_DIR=${SOURCE_DIR}/build3
BUILD_LIFTING_DIR=${BUILD_DIR}/tests/Lifting
EMSCRIPTEN_BIN=${HOME}/emsdk/upstream/emscripten
EMCC=${EMSCRIPTEN_BIN}/emcc
CXX=clang++-16
CXX_X64=x86_64-linux-gnu-g++-11
EMCCFLAGS="-DREMILL_DISABLE_INT128 -I${SOURCE_DIR}/include"
CLANGFLAGS="-g -static"
X64_GCC_FLAGS="-g -static -I${SOURCE_DIR}/include"
ONLY_BUILD=0

echo "[INFO] build start.\n"

if [ $# -ne 1 ]; then
    echo "[ERROR] target ELF binary is not specified."
    echo $#
    exit 1
fi

# build Lift.cpp
cd $BUILD_DIR && \
    ./tests/Lifting/lifting_target_aarch64 \
    --arch aarch64 \
    --bc_out ${BUILD_LIFTING_DIR}/lifting_target_aarch64.bc \
    --target_elf $1 && \
    llvm-dis ${BUILD_LIFTING_DIR}/lifting_target_aarch64.bc -o ${BUILD_LIFTING_DIR}/lifting_target_aarch64.ll
echo "[INFO] generate lifting_target_aarch64.ll\n"

# generate executable by clang (target: aarch64)
# cd $SOURCE_LIFTING_DIR && \
#     $CXX $CLANGFLAGS -S -emit-llvm -o ${BUILD_LIFTING_DIR}/Entry.clang.ll Entry.cpp && cd $BUILD_LIFTING_DIR && \
#     $CXX $CLANGFLAGS -c -o Entry.clang.o Entry.clang.ll && $CXX $CLANGFLAGS -c -o lifting_target_aarch64.clang.o lifting_target-aarch64.ll && \
#     $CXX $CLANGFLAGS -o exe Entry.clang.o lifting_target_aarch64.clang.o

# generate executable by cross-gcc (target: x86_64)
# cd $SOURCE_LIFTING_DIR && \
#     $CXX_X64 $X64_GCC_FLAGS -S -emit-llvm -o ${BUILD_LIFTING_DIR}/Entry.x64_gcc.ll Entry.cpp && cd $BUILD_LIFTING_DIR && \
#     $CXX_X64 $X64_GCC_FLAGS -c -o Entry.x64_gcc.o Entry.x64_gcc.ll && $CXX_X64 $X64_GCC_FLAGS -c -o lifting_target_aarch64.x64_gcc.o lifting_target-aarch64.ll && \
#     $CXX_X64 $X64_GCC_FLAGS -o exe_x64 Entry.x64_gcc.o lifting_target_aarch64.x64_gcc.o

# generate executable by emscripten (target: wasm)
if [ $ONLY_BUILD -ne 1 ]; then
    cd $SOURCE_LIFTING_DIR && \
        $EMCC $EMCCFLAGS -c Entry.cpp -o ${BUILD_LIFTING_DIR}/Entry.o && \
        $EMCC $EMCCFLAGS -c Syscall.cpp -o ${BUILD_LIFTING_DIR}/Syscall.o && \
        $EMCC $EMCCFLAGS -c VmIntrinsics.cpp -o ${BUILD_LIFTING_DIR}/VmIntrinsics.o && \
        cd $BUILD_LIFTING_DIR && \
        $EMCC $EMCCFLAGS -c lifting_target_aarch64.ll -o lifting_target_aarch64.o && \
        $EMCC $EMCCFLAGS -o exe.wasm Entry.o Syscall.o VmIntrinsics.o lifting_target_aarch64.o
fi