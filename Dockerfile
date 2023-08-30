# Choose your LLVM version
ARG LLVM_VERSION=16
ARG ARCH=aarch64
ARG UBUNTU_VERSION=22.04
ARG DISTRO_BASE=ubuntu${UBUNTU_VERSION}
ARG BUILD_BASE=ubuntu:${UBUNTU_VERSION}
ARG LIBRARIES=/opt/trailofbits


# Run-time dependencies go here
FROM ${BUILD_BASE} as base

# Build-time dependencies go here
# See here for full list of those dependencies
# https://github.com/lifting-bits/cxx-common/blob/master/docker/Dockerfile.ubuntu.vcpkg
FROM trailofbits/cxx-common-vcpkg-builder-ubuntu:${UBUNTU_VERSION}_arm64 as deps
ARG UBUNTU_VERSION
ARG ARCH
ARG LLVM_VERSION
ARG LIBRARIES

RUN date
RUN apt update

RUN apt install -qqy python3 python3-pip libc6-dev wget liblzma-dev zlib1g-dev curl git build-essential ninja-build libselinux1-dev libbsd-dev ccache pixz xz-utils make rpm

RUN rm -rf /var/lib/apt/lists/*

# Source code build
FROM deps as build
ARG LLVM_VERSION

WORKDIR /remill
COPY ./ ./

RUN git config --global user.email "41898282+github-actions[bot]@users.noreply.github.com" && git config --global user.name "github-actions[bot]"

RUN ./scripts/build.sh \
    --llvm-version ${LLVM_VERSION} \
    --prefix /opt/trailofbits \
    --extra-cmake-args "-DCMAKE_BUILD_TYPE=Release" \
    --disable-package

RUN pip3 install ./scripts/diff_tester_export_insns

RUN cd remill-build && \
    cmake --build . --target test_dependencies -- -j $(nproc) && \
    CTEST_OUTPUT_ON_FAILURE=1 cmake --build . --verbose --target test -- -j $(nproc) && \
    cmake --build . --target install

# Small installation image
FROM base as install
ARG LLVM_VERSION

COPY --from=build /opt/trailofbits /opt/trailofbits
COPY scripts/docker-lifter-entrypoint.sh /opt/trailofbits
ENV LLVM_VERSION=llvm${LLVM_VERSION} \
    PATH=/opt/trailofbits/bin
ENTRYPOINT ["/opt/trailofbits/docker-lifter-entrypoint.sh"]
