# ofxSherpaOnnx

openFrameworks addon for real-time speech-to-text using [sherpa-onnx](https://github.com/k2-fsa/sherpa-onnx).

## What is ofxSherpaOnnx?

ofxSherpaOnnx brings high-quality, real-time, offline speech recognition into the openFrameworks ecosystem. It acts as a C++ wrapper for the `sherpa-onnx` library, allowing for easy integration of voice control and transcription into creative coding projects.

This makes it possible to build voice-activated installations, experimental interfaces, and real-time generative applications without relying on cloud services or Python.

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

## Setup


### 1. Build onnx-sherpa library with CMake

Navigate to the `scripts` folder and execute the setup script to build onnx-sherpa as a static library:
```bash
chmod +x build_onnx-sherpa_static.sh
./build_onnx-sherpa_static.sh
```


### 2. Download the ASR Model

The example_basics requires a pre-trained automatic speech recognition (ASR) model to function. A script is provided to download the model used in the example.

Navigate to the `scripts` folder and execute the download script:
```bash
chmod +x download_model.sh
./download_model.sh
```
This will download and extract an exemplary model into the `example_basics/bin/data/models/` directory.

### 3. Build and Run the Example

Once the model is in place, you can build and run the example project.

Navigate into the example folder and compile the openFrameworks example:
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
