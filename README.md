# ofxSherpaOnnx

openFrameworks addon for real-time speech-to-text and text-to-speech using [sherpa-onnx](https://github.com/k2-fsa/sherpa-onnx).

## What is ofxSherpaOnnx?

ofxSherpaOnnx brings high-quality, real-time, offline speech recognition and synthesis into the openFrameworks ecosystem. It acts as a C++ wrapper for the `sherpa-onnx` library, allowing for easy integration of voice control, transcription, and speech generation into creative coding projects.

This makes it possible to build speech-enabled installations, experimental interfaces, and real-time generative applications without relying on cloud services.

## Tested Environments

ofxSherpaOnnx has been validated on:

* x86_64 (Ubuntu 24.04.3 LTS)
* ARM 64-bit (Raspberry Pi)
* macOS (Apple Silicon M2)
* Windows 11
* openFrameworks:
  * `of_v0.12.1_linux64_gcc6_release`

## Dependencies

* **sherpa-onnx**
  High-performance, streaming, and non-streaming speech recognition in pure C/C++ with ONNX Runtime.
  [https://github.com/k2-fsa/sherpa-onnx](https://github.com/k2-fsa/sherpa-onnx)

### `example_tts` Dependencies

These dependencies are only required by `example_tts`. The ASR examples use openFrameworks and ofxSherpaOnnx without these additional addons.

* **ofxSoundObjects** (developed by [roymacdonald](https://github.com/roymacdonald)) is used to handle audio playback.
  [https://github.com/roymacdonald/ofxSoundObjects](https://github.com/roymacdonald/ofxSoundObjects)
* **ofxAudioFile** (developed by [roymacdonald](https://github.com/roymacdonald)) is a dependency of ofxSoundObjects and is used for audio file operations.
  [https://github.com/roymacdonald/ofxAudioFile](https://github.com/roymacdonald/ofxAudioFile)
* **ofxGui** is used for the GUI. It is part of the openFrameworks core addons.

## Setup

### 1. Install Dependencies and Models

From the addon root, run:

```bash
python3 scripts/install_all.py
```

On Windows, use `python` instead if Python is installed under that command:

```powershell
python scripts/install_all.py
```

The installer automatically:

1. Installs ONNX Runtime into the addon.
2. Builds sherpa-onnx as a static library.
3. Downloads the English and German ASR models for `example_asr` and `example_asr_buffer`.
4. Downloads the English and German TTS models for `example_tts`.

### 2. Build and Run the Examples

**Ubuntu / Debian (Linux) and macOS:**

Once the installer has completed, navigate into an example folder such as `example_asr`, `example_asr_buffer`, or `example_tts` and compile it:

```bash
make
```

Run the release executable:

```bash
make RunRelease
```
### Select the Example Language

All examples use German by default. On Linux and macOS, start an example explicitly in German with:

```bash
OFX_SHERPAONNX_LANG=de make RunRelease
```

To start an example in English, use:

```bash
OFX_SHERPAONNX_LANG=en make RunRelease
```

The language is selected when the application starts and determines which installed ASR or TTS model is loaded. If the requested model is unavailable, the examples automatically fall back to the other installed language model.

**Windows:**

Generate or update the example projects using the openFrameworks Project Generator. Open the generated Visual Studio solution (`.sln`), then build and run the desired example from Visual Studio.

## License

Copyright (c) 2026 Yannick Hofmann.

BSD Simplified License.

For information on usage and redistribution, and for a DISCLAIMER OF ALL WARRANTIES, see the file `LICENSE.txt` in this distribution.
