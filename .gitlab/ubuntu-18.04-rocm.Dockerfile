FROM streamhpc/opencl-sdk-base:ubuntu-18.04-20211119
RUN set -ex; \
    export DEBIAN_FRONTEND=noninteractive ; \
#   Register ROCm APT repo
    wget --quiet --recursive --no-directories --no-parent "https://repo.radeon.com/amdgpu-install/latest/ubuntu/bionic/" --accept "amdgpu-install-*_all.deb" ; \
    apt install -y -qq ./amdgpu-install-*_all.deb libnuma-dev initramfs-tools ; \
    apt update -qq; \
    rm ./amdgpu-install-*_all.deb ; \
#   Install OpenCL package only (and dependency)
    amdgpu-install -y --usecase=opencl