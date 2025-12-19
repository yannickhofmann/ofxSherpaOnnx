# ofxSherpaOnnx

openFrameworks addon for real-time speech-to-text and text-to-speech using [sherpa-onnx](https://github.com/k2-fsa/sherpa-onnx).

## What is ofxSherpaOnnx?

ofxSherpaOnnx brings high-quality, real-time, offline speech recognition and synthesis into the openFrameworks ecosystem. It acts as a C++ wrapper for the `sherpa-onnx` library, allowing for easy integration of voice control, transcription, and speech generation into creative coding projects.

This makes it possible to build speech-enabled installations, experimental interfaces, and real-time generative applications without relying on cloud services or Python.

## Tested Environments

ofxSherpaOnnx has been validated on:

*   Ubuntu 24.04.3 LTS
*   macOS (Apple Silicon M2)
*   openFrameworks:
    *   `of_v0.12.0_linux64gcc6_release`
    *   `of_v0.12.1_linux64_gcc6_release`


## Dependencies

*   **sherpa-onnx**  
    High-performance, streaming, and non-streaming speech recognition in pure C/C++ with ONNX runtime.
    [https://github.com/k2-fsa/sherpa-onnx](https://github.com/k2-fsa/sherpa-onnx)

### Build Tool Dependency

* **CMake**  
  CMake is required to configure and build sherpa-onnx as a static library, which is then used by ofxSherpaOnnx. CMake is a cross-platform build system and must be installed before attempting to build these libraries.

**Ubuntu / Debian (Linux):**

```bash
sudo apt update
sudo apt install cmake
```

**macOS (OS X):**

```bash
brew install cmake
```

### Example Dependencies

* **ofxSoundObjects** is used in the **example_tts** to handle audio playback.
    [https://github.com/roymacdonald/ofxSoundObjects](https://github.com/roymacdonald/ofxSoundObjects)
* **ofxAudioFile** is a dependency of ofxSoundObjects and is used for audio file operations.
    [https://github.com/roymacdonald/ofxAudioFile](https://github.com/roymacdonald/ofxAudioFile)
* **ofxGui** is used in both examples for the GUI. It is part of the openFrameworks core addons.


## Setup


### 1. Build sherpa-onnx library with CMake

Navigate to the `scripts` folder and execute the setup script to build sherpa-onnx as a static library:
```bash
chmod +x build_sherpa-onnx_static.sh
./build_sherpa-onnx_static.sh
```


### 2. Download the ASR and TTS Models

The examples require pre-trained models to function. Scripts are provided to download the models used in the examples.

#### For `example_asr`:

Navigate to the `scripts` folder and execute the download script:
```bash
chmod +x download_ASR_model.sh
./download_ASR_model.sh
```
This will download and extract an exemplary model into the `example_asr/bin/data/models/` directory.

#### For `example_tts`:

Navigate to the `scripts` folder and execute the download script:
```bash
chmod +x download_TTS_model.sh
./download_TTS_model.sh
```
This will download and extract an exemplary model into the `example_tts/bin/data/models/` directory.


### 3. Build and Run the Examples

Once the models are in place, you can build and run the example projects. The example does not include a Makefile. Generate the project using the openFrameworks Project Generator as usual. Then navigate into the example folder (e.g., `example_asr` or `example_tts`) and compile the openFrameworks example:
```bash
make
```

Run the release executable:
```bash
make RunRelease
```


## License

Copyright (c) 2025 Yannick Hofmann.

BSD Simplified License.

For information on usage and redistribution, and for a DISCLAIMER OF ALL WARRANTIES, see the file, "LICENSE.txt," in this distribution.