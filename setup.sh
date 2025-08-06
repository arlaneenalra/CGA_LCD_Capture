#!/usr/bin/env bash
set -e

# Find the directory of the script file
# See: https://stackoverflow.com/a/246128
DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )

export PICO_SDK_PATH=${DIR}/pico-sdk
export PICO_EXTRAS_PATH=${DIR}/pico-extras
export PICO_DVI_PATH=${DIR}/PicoDVI/software


export BUILD_TYPE=Release

BUILD_DIR=${DIR}/build



get_core_count() {
  if [[ "$(uname -s || true)" == "Darwin" ]]; then
    sysctl -n hw.ncpu
  else
    cat /proc/cpuinfo | grep processor | wc -l
  fi
}

setup() {
  PICO_BOARD=${PICO_BOARD:-pico}
  DVI_DEFAULT_SERIAL_CONFIG=${DVI_DEFAULT_SERIAL_CONFIG:-pico_sock_cfg}

  export PICO_PLATFORM
  export PICO_BOARD
  export DVI_DEFAULT_SERIAL_CONFIG 
  
  TARGET_DIR_DEFAULT=${PICO_PLATFORM:-$PICO_BOARD}
  TARGET_DIR=${TARGET_DIR:-$TARGET_DIR_DEFAULT}

  export BUILD_DIR=${DIR}/build/${TARGET_DIR}

  echo "Building for ${TARGET_DIR}" 

  cmake -E make_directory ${BUILD_DIR}

  (
    cd  ${BUILD_DIR}

    cmake ${DIR} \
      -DCMAKE_BUILD_TYPE=${BUILD_TYPE} \
      -DPICO_COPY_TO_RAM=1 \
      -DDVI_DEFAULT_SERIAL_CONFIG=${DVI_DEFAULT_SERIAL_CONFIG} \
      -DPICO_BOARD=${PICO_BOARD}

    cmake --build . --config $BUILD_TYPE --parallel $(get_core_count)
  )
}

# Doesn't work yet.
#TARGET_DIR=rp2040-feather PICO_BOARD=adafruit_feather_rp2040 PICO_PLATFORM=rp2040 DVI_DEFAULT_SERIAL_CONFIG=adafruit_feather_dvi_cfg setup 
TARGET_DIR=rp2040 PICO_PLATFORM=rp2040 setup 
PICO_PLATFORM=rp2350 PICO_BOARD=pico2 DVI_DEFAULT_SERIAL_CONFIG=pico_sock_cfg setup 
