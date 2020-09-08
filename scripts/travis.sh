#!/usr/bin/env bash

set -e

if [ "${JOB_CHECK_FORMAT}" -eq 1 ]; then
    # Check format and exit early
    "${TRAVIS_BUILD_DIR}/scripts/check-format.sh"
    exit $?
fi

# Build SDK
mkdir build
cd build
cmake ..
make
