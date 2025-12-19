#!/usr/bin/env bash
set -euo pipefail

# This script downloads the necessary models for the ofxSherpaOnnx Text-to-Speech example.
#
# Usage:
# ./download_TTS_model.sh

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ADDON_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
MODEL_DIR="$ADDON_ROOT/example_tts/bin/data/models/vits-piper-en_US-amy-low"
mkdir -p "$MODEL_DIR"

BASE_URL="https://huggingface.co/csukuangfj/vits-piper-en_US-amy-low/resolve/main"

echo "Downloading TTS model..."
wget -q --show-progress -O "$MODEL_DIR/model.onnx" "$BASE_URL/en_US-amy-low.onnx"
wget -q --show-progress -O "$MODEL_DIR/lexicon.txt" "$BASE_URL/lexicon.txt" || true
wget -q --show-progress -O "$MODEL_DIR/tokens.txt" "$BASE_URL/tokens.txt"
wget -q --show-progress -O "$MODEL_DIR/en_US-amy-low.onnx.json" "$BASE_URL/en_US-amy-low.onnx.json"

for f in model.onnx lexicon.txt tokens.txt en_US-amy-low.onnx.json; do
  if [ ! -s "$MODEL_DIR/$f" ] && [ "$f" != "lexicon.txt" ]; then
    echo "Download failed or empty file: $MODEL_DIR/$f" >&2
    exit 1
  fi
done

if [ ! -s "$MODEL_DIR/lexicon.txt" ]; then
  echo "Lexicon missing or empty; fetching espeak-ng-data for phonemizer..."
  if ! command -v git >/dev/null 2>&1; then
    echo "git is required to fetch espeak-ng-data (install git or supply a lexicon)." >&2
    exit 1
  fi
  TMP_DIR="$(mktemp -d)"
  git clone --filter=blob:none --sparse https://huggingface.co/csukuangfj/vits-piper-en_US-amy-low "$TMP_DIR"
  git -C "$TMP_DIR" sparse-checkout set espeak-ng-data
  rm -rf "$MODEL_DIR/espeak-ng-data"
  mv "$TMP_DIR/espeak-ng-data" "$MODEL_DIR/"
  rm -rf "$TMP_DIR"
fi

echo "TTS model download complete."
