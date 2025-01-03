#!/usr/bin/env bash
set -e

# Find the directory of the script file
# See: https://stackoverflow.com/a/246128
DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )

export PICO_SDK_PATH=${DIR}/pico-sdk
export PICO_EXTRAS_PATH=${DIR}/pico-extras

export BUILD_TYPE=Release
export BUILD_DIR=${DIR}/build
export PICO_PLATFORM=rp2040



get_core_count() {
  if [[ "$(uname -s || true)" == "Darwin" ]]; then
    sysctl -n hw.ncpu
  else
    cat /proc/cpuinfo | grep processor | wc -l
  fi
}

cmake -E make_directory ${BUILD_DIR}

(
  cd  ${BUILD_DIR}

  cmake .. -DCMAKE_BUILD_TYPE=${BUILD_TYPE}
  cmake --build . --config $BUILD_TYPE --parallel $(get_core_count)
)


