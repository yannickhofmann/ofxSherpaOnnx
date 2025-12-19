#!/bin/bash
#
# This script downloads the required model for the ofxSherpaOnnx example.
# It should be run from the root of the addon (`addons/ofxSherpaOnnx`).
#

set -e

# Get the directory of the script itself
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# The data directory for the example is relative to the script's location
DATA_DIR="${SCRIPT_DIR}/../example_asr/bin/data"

MODELS_DIR="${DATA_DIR}/models"

MODEL_URL="https://github.com/k2-fsa/sherpa-onnx/releases/download/asr-models/sherpa-onnx-streaming-zipformer-bilingual-zh-en-2023-02-20.tar.bz2"
ARCHIVE_NAME=$(basename "${MODEL_URL}")
# The final directory name expected by ofApp.cpp
MODEL_DIR_NAME="online-zipformer-bilingual-zh-en-2023-02-20"

# --- Create data directory if it doesn't exist ---
mkdir -p "${MODELS_DIR}"


echo "---"
echo "Target models directory: ${MODELS_DIR}"
echo "---"


# --- Check if model already exists ---
if [ -d "${MODELS_DIR}/${MODEL_DIR_NAME}" ]; then
  echo "--- Model directory '${MODEL_DIR_NAME}' already exists. Skipping download. ---"
  exit 0
fi

# --- Download ---
# We'll download the archive to a temporary location inside the models dir
cd "${MODELS_DIR}"
echo "--- Downloading ${ARCHIVE_NAME} ---"
if command -v wget &> /dev/null; then
  wget -c "${MODEL_URL}"
else
  echo "wget not found. Trying curl..."
  if command -v curl &> /dev/null; then
    curl -L -O "${MODEL_URL}"
  else
    echo "Error: Neither wget nor curl is installed. Please install one and try again."
    exit 1
  fi
fi

# --- Extract ---
echo "--- Extracting ${ARCHIVE_NAME} ---"
# Create the target directory first
mkdir -p "${MODEL_DIR_NAME}"
tar -xvf "${ARCHIVE_NAME}" -C "${MODEL_DIR_NAME}" --strip-components=1

# --- Cleanup ---
echo "--- Cleaning up ---"
rm "${ARCHIVE_NAME}"

echo "---"
echo "Done!"
echo "Model downloaded and extracted to ${MODELS_DIR}/${MODEL_DIR_NAME}"
echo "---"
