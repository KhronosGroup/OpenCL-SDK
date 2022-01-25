FROM ubuntu:18.04 AS apt-installs
RUN set -ex; \
    export DEBIAN_FRONTEND=noninteractive ; \
    apt update -qq; \
#   install wget to download repository keys and CMake tarballs
#   install software-properties-common for the apt-add-repository command
    apt install -y -qq wget software-properties-common ; \
#   Canonical hosts recent GCC compilers in ubuntu-toolchain-r/test
    apt-add-repository -y ppa:ubuntu-toolchain-r/test ; \
#   LLVM hosts most toolchain in separate repos. We only register those absent from ubuntu-toolchain-r/test
    wget -q -O - https://apt.llvm.org/llvm-snapshot.gpg.key | apt-key add - ; \
    apt-add-repository -y 'deb [arch=amd64] https://apt.llvm.org/bionic/ llvm-toolchain-bionic-11 main' ; \
    apt-add-repository -y 'deb [arch=amd64] https://apt.llvm.org/bionic/ llvm-toolchain-bionic-12 main' ; \
    apt-add-repository -y 'deb [arch=amd64] https://apt.llvm.org/bionic/ llvm-toolchain-bionic-13 main' ; \
#   install ninja, GCC 7-10, LLVM 8-13 and build-essential to get linkers, etc.
#   install git to download dependencies
#   install ruby to run CMock
#   install libidn11 which CMake 3.1.3 only depends on
#   install ca-certificates to `git clone` via HTTPS
#   install SFML dependencies
#           libx11-dev libxrandr-dev libxi-dev
#           mesa-common-dev for gl.h
#           libgl1-mesa-dev for libGL.so
#           libglu1-mesa-dev for glu.h
#           libudev-dev
#   install Vcpkg dependencies
#           curl zip unzip tar
    apt install -y -qq build-essential g++-7 g++-8 g++-9 g++-10 clang-8 clang-9 clang-10 clang-11 clang-12 clang-13 ninja-build git ruby libidn11 ca-certificates libx11-dev libxrandr-dev libxi-dev mesa-common-dev libgl1-mesa-dev libglu1-mesa-dev libudev-dev curl zip unzip tar

#   install CMake minimum (3.0.2 (Headers, ICD Loader), 3.1.3 (CLHPP), 3.10.3 (SDK)) and latest (3.21.2)
RUN mkdir -p /opt/Kitware/CMake ; \
    wget -c https://github.com/Kitware/CMake/releases/download/v3.0.2/cmake-3.0.2-Linux-i386.tar.gz -O - | tar -xz --directory /opt/Kitware/CMake ; \
    mv /opt/Kitware/CMake/cmake-3.0.2-Linux-i386 /opt/Kitware/CMake/3.0.2 ; \
    wget -c https://github.com/Kitware/CMake/releases/download/v3.1.3/cmake-3.1.3-Linux-x86_64.tar.gz -O - | tar -xz --directory /opt/Kitware/CMake ; \
    mv /opt/Kitware/CMake/cmake-3.1.3-Linux-x86_64 /opt/Kitware/CMake/3.1.3 ; \
    wget -c https://github.com/Kitware/CMake/releases/download/v3.10.3/cmake-3.10.3-Linux-x86_64.tar.gz -O - | tar -xz --directory /opt/Kitware/CMake ; \
    mv /opt/Kitware/CMake/cmake-3.10.3-Linux-x86_64 /opt/Kitware/CMake/3.10.3 ; \
    wget -c https://github.com/Kitware/CMake/releases/download/v3.21.2/cmake-3.21.2-linux-x86_64.tar.gz -O - | tar -xz --directory /opt/Kitware/CMake ; \
    mv /opt/Kitware/CMake/cmake-3.21.2-linux-x86_64 /opt/Kitware/CMake/3.21.2

#   install Vcpkg
RUN git clone --depth 1 https://github.com/Microsoft/vcpkg.git /opt/Microsoft/vcpkg ; \
    /opt/Microsoft/vcpkg/bootstrap-vcpkg.sh ; \
#   install SFML, TCLAP, GLM
    /opt/Microsoft/vcpkg/vcpkg install sfml tclap glm glew