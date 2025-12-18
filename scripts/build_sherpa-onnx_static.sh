#!/bin/bash
#
# This script downloads and builds a static version of the sherpa-onnx library.
# It should be run from the root of the addon (`addons/ofxSherpaOnnx`).
#

set -e

# Get the directory of the script itself
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ADDON_ROOT="${SCRIPT_DIR}/.."

# The final install directory for the library, organized under a 'sherpa-onnx' subfolder
INSTALL_DIR="${ADDON_ROOT}/libs/sherpa-onnx"

# Temporary directories for building
SRC_DIR="${ADDON_ROOT}/sherpa-onnx-src"
BUILD_DIR="${SRC_DIR}/build"

# --- Platform Detection ---
OS="$(uname -s)"
ARCH="$(uname -m)"

echo "---"
echo "Building for OS: ${OS} (${ARCH})"
echo "Installing to: ${INSTALL_DIR}"
echo "---"

# --- Dependency Check ---
# (omitted for brevity, same as before)

# --- Cloning ---
if [ -d "${SRC_DIR}" ]; then
  echo "--- Source directory already exists. Skipping clone. ---"
else
  echo "--- Cloning sherpa-onnx ---"
  git clone https://github.com/k2-fsa/sherpa-onnx.git "${SRC_DIR}"
fi

# --- Building ---
echo "--- Configuring build (static library) ---"
mkdir -p "${BUILD_DIR}"
cd "${BUILD_DIR}"

cmake_extra_defs=""
if [ "${OS}" == "Darwin" ]; then
  cmake_extra_defs="-DCMAKE_OSX_DEPLOYMENT_TARGET=10.14"
fi

cmake ../ \
  -DCMAKE_INSTALL_PREFIX=./install \
  -DBUILD_SHARED_LIBS=OFF \
  -DSHERPA_ONNX_ENABLE_PYTHON=OFF \
  -DSHERPA_ONNX_ENABLE_TESTS=OFF \
  -DSHERPA_ONNX_ENABLE_BINARY=OFF \
  -DSHERPA_ONNX_ENABLE_C_API=ON \
  ${cmake_extra_defs}

echo "--- Compiling ---"
if [ "${OS}" == "Linux" ]; then
  make -j$(nproc)
elif [ "${OS}" == "Darwin" ]; then
  make -j$(sysctl -n hw.ncpu)
else
  make
fi

echo "--- Installing to temporary directory ---"
make install

# --- Copying to final destination ---
echo "--- Copying files to ${INSTALL_DIR} ---"
mkdir -p "${INSTALL_DIR}/include"
mkdir -p "${INSTALL_DIR}/lib/${OS}_${ARCH}"

echo "Copying headers..."
# We want to copy the contents of install/include, not the folder itself
cp -R "${BUILD_DIR}/install/include/"* "${INSTALL_DIR}/include/"

echo "Copying libraries..."
cp "${BUILD_DIR}/install/lib/"*.a "${INSTALL_DIR}/lib/${OS}_${ARCH}/"
# also copy onnxruntime lib from the build deps
cp "${BUILD_DIR}/_deps/onnxruntime-src/lib/"*.a "${INSTALL_DIR}/lib/${OS}_${ARCH}/"


# --- Cleanup ---
echo "--- Cleaning up ---"
rm -rf "${SRC_DIR}"

echo "---"
echo "Done!"
echo "Static libraries and headers are in ${INSTALL_DIR}"
echo "---"