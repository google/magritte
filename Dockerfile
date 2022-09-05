#
# Copyright 2020-2022 Google LLC
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
# Dockerfile for Magritte build environment.
FROM ubuntu:18.04

################################################################################
# Core packages required to fetch toolchains
################################################################################
RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential \
    ca-certificates \
    libopencv-core-dev \
    libopencv-highgui-dev \
    libopencv-imgproc-dev \
    libopencv-video-dev \
    libopencv-calib3d-dev \
    libopencv-features2d-dev \
    software-properties-common \
    sudo \
    wget \
    unzip \
    && \
    rm -rf /var/lib/apt/lists/*

################################################################################
# using gcc-7
# using gcc-8
# using gcc-9
################################################################################
RUN sudo add-apt-repository ppa:ubuntu-toolchain-r/test
RUN apt-get update && apt-get install -y \
    gcc-7 g++-7 gcc-7-multilib g++-7-multilib \
    gcc-8 g++-8 gcc-8-multilib g++-8-multilib \
    gcc-9 g++-9 gcc-9-multilib g++-9-multilib \
    && rm -rf /var/lib/apt/lists/* && \
    update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-7 70 \
        --slave /usr/bin/g++  g++  /usr/bin/g++-7 \
        --slave /usr/bin/gcov gcov /usr/bin/gcov-7 \
        && \
    update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-8 80 \
        --slave /usr/bin/g++  g++  /usr/bin/g++-8 \
        --slave /usr/bin/gcov gcov /usr/bin/gcov-8 \
        && \
    update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-9 90 \
        --slave /usr/bin/g++  g++  /usr/bin/g++-9 \
        --slave /usr/bin/gcov gcov /usr/bin/gcov-9

################################################################################
# Extra packages (incl. Android SDK & sdkmanager)
################################################################################
RUN apt-get update && apt-get install -y \
    android-sdk \
    git \
    lcov \
    libx11-dev \
    libxext-dev \
    pkg-config \
    python \
    python3-distutils \
    python3-numpy \
    zlib1g-dev \
    && \
    rm -rf /var/lib/apt/lists/* && \
    wget https://dl.google.com/android/repository/commandlinetools-linux-7583922_latest.zip && \
    unzip commandlinetools-linux-7583922_latest.zip && \
    sudo mv cmdline-tools /usr/lib/android-sdk/ && \
    yes | /usr/lib/android-sdk/cmdline-tools/bin/sdkmanager --sdk_root=/usr/lib/android-sdk "platforms;android-30" "build-tools;30.0.3"
ENV ANDROID_HOME=/usr/lib/android-sdk

################################################################################
# using ninja-1.10.0
################################################################################
WORKDIR /bin
RUN wget https://github.com/ninja-build/ninja/releases/download/v1.10.0/ninja-linux.zip && \
    unzip ninja-linux.zip -d ninja-1.10.0 && \
    rm ninja-linux.zip

################################################################################
# using cmake-3.17.2
################################################################################
WORKDIR /bin
RUN wget https://github.com/Kitware/CMake/releases/download/v3.17.2/cmake-3.17.2-Linux-x86_64.tar.gz && \
    tar -xzvf cmake-3.17.2-Linux-x86_64.tar.gz > /dev/null && \
    mv cmake-3.17.2-Linux-x86_64 cmake-3.17.2 && \
    rm cmake-3.17.2-Linux-x86_64.tar.gz

################################################################################
# using clang-8.0.0
################################################################################
WORKDIR /bin
RUN wget https://releases.llvm.org/8.0.0/clang+llvm-8.0.0-x86_64-linux-gnu-ubuntu-18.04.tar.xz && \
    tar -xf clang+llvm-8.0.0-x86_64-linux-gnu-ubuntu-18.04.tar.xz > /dev/null && \
    mv clang+llvm-8.0.0-x86_64-linux-gnu-ubuntu-18.04 clang-8.0.0 && \
    rm clang+llvm-8.0.0-x86_64-linux-gnu-ubuntu-18.04.tar.xz

################################################################################
# using clang-10.0.0
################################################################################
WORKDIR /bin
RUN wget https://github.com/llvm/llvm-project/releases/download/llvmorg-10.0.0/clang+llvm-10.0.0-x86_64-linux-gnu-ubuntu-18.04.tar.xz && \
    tar -xf clang+llvm-10.0.0-x86_64-linux-gnu-ubuntu-18.04.tar.xz > /dev/null && \
    mv clang+llvm-10.0.0-x86_64-linux-gnu-ubuntu-18.04 clang-10.0.0 && \
    rm clang+llvm-10.0.0-x86_64-linux-gnu-ubuntu-18.04.tar.xz

################################################################################
# using bazel-5.2.0
################################################################################
WORKDIR /bin
RUN wget https://github.com/bazelbuild/bazel/releases/download/5.2.0/bazel-5.2.0-linux-x86_64 && \
    mkdir bazel-5.2.0 && \
    mv bazel-5.2.0-linux-x86_64 bazel-5.2.0/bazel && \
    chmod +x bazel-5.2.0/bazel

################################################################################
# using ndk-r21e
################################################################################
WORKDIR /bin
RUN wget https://dl.google.com/android/repository/android-ndk-r21e-linux-x86_64.zip && \
    mkdir -p android/ndk && \
    unzip android-ndk-r21e-linux-x86_64.zip -d extracted && \
    mv extracted/android-ndk-r21e android/ndk-r21e && \
    rm -fr extracted && \
    rm android-ndk-r21e-linux-x86_64.zip
ENV ANDROID_NDK_HOME=/bin/android/ndk-r21e

################################################################################
# using go-1.14.4
################################################################################
WORKDIR /bin
RUN wget https://golang.org/dl/go1.14.4.linux-amd64.tar.gz && \
    tar -xzvf go1.14.4.linux-amd64.tar.gz > /dev/null && \
    mv go go-1.14.4 && \
    rm go1.14.4.linux-amd64.tar.gz

################################################################################
# mingw-w64 toolchain
################################################################################
RUN apt-get update && apt-get install -y \
    gcc-mingw-w64 \
    gcc-mingw-w64-base \
    gcc-mingw-w64-i686 \
    gcc-mingw-w64-x86-64 \
    g++-mingw-w64 \
    g++-mingw-w64-i686 \
    g++-mingw-w64-x86-64 \
    && \
    rm -rf /var/lib/apt/lists/*

################################################################################
# depot-tools
################################################################################
WORKDIR /bin
RUN git clone https://chromium.googlesource.com/chromium/tools/depot_tools.git

################################################################################
# doxygen-1.8.18
################################################################################
WORKDIR /bin
RUN wget https://sourceforge.net/projects/doxygen/files/rel-1.8.18/doxygen-1.8.18.linux.bin.tar.gz/download -O doxygen.tar.gz && \
    tar -xzvf doxygen.tar.gz > /dev/null && \
    rm doxygen.tar.gz

################################################################################
# Toolchain environment scripts
################################################################################
RUN ln -s /bin/bazel-5.2.0/bazel /bin/bazel

################################################################################
# Magritte Requirements
################################################################################
RUN apt-get update && apt-get install -y \
    libopencv-core-dev \
    libopencv-highgui-dev \
    libopencv-calib3d-dev \
    libopencv-features2d-dev \
    libopencv-imgproc-dev \
    libopencv-video-dev \
    mesa-common-dev \
    libegl1-mesa-dev \
    libgles2-mesa-dev

ARG username
ARG groupname
ARG uid
ARG gid
ARG home

RUN groupadd --gid "${gid}" "${groupname}" \
  && useradd --home-dir "${home}" --gid "${gid}" --uid "${uid}" "${username}"

USER "${username}"

LABEL Name=ubuntu-build Version=0.0.8
