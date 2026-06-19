# ofxSherpaOnnx addon_config.mk
# Cross-platform configuration for Linux, macOS and Windows

meta:
	ADDON_NAME = ofxSherpaOnnx
	ADDON_DESCRIPTION = openFrameworks addon for real-time speech-to-text and text-to-speech using sherpa-onnx
	ADDON_AUTHOR = Yannick Hofmann
	ADDON_TAGS = "ai", "speech-to-text", "text-to-speech", "nlp", "sherpa-onnx", "openframeworks", "ofxaddon"
	ADDON_URL = https://github.com/yannickhofmann/ofxSherpaOnnx

common:
	ADDON_SOURCES = src/ofxSherpaOnnx.cpp
	ADDON_INCLUDES = src
	ADDON_INCLUDES += libs/sherpa-onnx/include
	ADDON_INCLUDES += libs/onnxruntime/include

linux64:
	base_lib_dir = libs/sherpa-onnx/lib/Linux_x86_64
	ADDON_LDFLAGS += -L$(base_lib_dir)
	ADDON_LIBS += -Wl,--start-group
	ADDON_LIBS += $(base_lib_dir)/libsherpa-onnx-c-api.a
	ADDON_LIBS += $(base_lib_dir)/libsherpa-onnx-cxx-api.a
	ADDON_LIBS += $(base_lib_dir)/libsherpa-onnx-core.a
	ADDON_LIBS += $(base_lib_dir)/libkaldi-decoder-core.a
	ADDON_LIBS += $(base_lib_dir)/libsherpa-onnx-kaldifst-core.a
	ADDON_LIBS += $(base_lib_dir)/libkaldi-native-fbank-core.a
	ADDON_LIBS += $(base_lib_dir)/libsherpa-onnx-fst.a
	ADDON_LIBS += $(base_lib_dir)/libsherpa-onnx-fstfar.a
	ADDON_LIBS += $(base_lib_dir)/libssentencepiece_core.a
	ADDON_LIBS += $(base_lib_dir)/libpiper_phonemize.a
	ADDON_LIBS += $(base_lib_dir)/libespeak-ng.a
	ADDON_LIBS += $(base_lib_dir)/libucd.a
	ADDON_LIBS += $(base_lib_dir)/libkissfft-float.a
	ADDON_LIBS += $(base_lib_dir)/libonnxruntime.a
	ADDON_LIBS += -Wl,--end-group
	ADDON_LDFLAGS += -ldl -lpthread -lrt

linuxaarch64:
	base_lib_dir = libs/sherpa-onnx/lib/Linux_arm64
	ADDON_LDFLAGS += -L$(base_lib_dir)
	ADDON_LIBS += -Wl,--start-group
	ADDON_LIBS += $(base_lib_dir)/libsherpa-onnx-c-api.a
	ADDON_LIBS += $(base_lib_dir)/libsherpa-onnx-cxx-api.a
	ADDON_LIBS += $(base_lib_dir)/libsherpa-onnx-core.a
	ADDON_LIBS += $(base_lib_dir)/libkaldi-decoder-core.a
	ADDON_LIBS += $(base_lib_dir)/libsherpa-onnx-kaldifst-core.a
	ADDON_LIBS += $(base_lib_dir)/libkaldi-native-fbank-core.a
	ADDON_LIBS += $(base_lib_dir)/libsherpa-onnx-fst.a
	ADDON_LIBS += $(base_lib_dir)/libsherpa-onnx-fstfar.a
	ADDON_LIBS += $(base_lib_dir)/libssentencepiece_core.a
	ADDON_LIBS += $(base_lib_dir)/libpiper_phonemize.a
	ADDON_LIBS += $(base_lib_dir)/libespeak-ng.a
	ADDON_LIBS += $(base_lib_dir)/libucd.a
	ADDON_LIBS += $(base_lib_dir)/libkissfft-float.a
	ADDON_LIBS += $(base_lib_dir)/libonnxruntime.a
	ADDON_LIBS += -Wl,--end-group
	ADDON_LDFLAGS += -ldl -lpthread -lrt

osx:
	# openFrameworks selects the generic "osx" section for both Intel and
	# Apple Silicon. The installer stores libraries in an architecture-specific
	# directory, so derive that directory from OF's PLATFORM_ARCH.
	base_lib_dir = libs/sherpa-onnx/lib/Darwin_$(PLATFORM_ARCH)
	# Replace OF's automatically discovered ONNX Runtime dylibs. sherpa-onnx
	# was built against the static ONNX Runtime copied into this same directory;
	# mixing that with the separately downloaded dylib can produce ABI/version
	# mismatches.
	ADDON_LIBS = $(base_lib_dir)/libsherpa-onnx-c-api.a
	ADDON_LIBS += $(base_lib_dir)/libsherpa-onnx-cxx-api.a
	ADDON_LIBS += $(base_lib_dir)/libsherpa-onnx-core.a
	ADDON_LIBS += $(base_lib_dir)/libkaldi-decoder-core.a
	ADDON_LIBS += $(base_lib_dir)/libsherpa-onnx-kaldifst-core.a
	ADDON_LIBS += $(base_lib_dir)/libkaldi-native-fbank-core.a
	ADDON_LIBS += $(base_lib_dir)/libsherpa-onnx-fst.a
	ADDON_LIBS += $(base_lib_dir)/libsherpa-onnx-fstfar.a
	ADDON_LIBS += $(base_lib_dir)/libssentencepiece_core.a
	ADDON_LIBS += $(base_lib_dir)/libpiper_phonemize.a
	ADDON_LIBS += $(base_lib_dir)/libespeak-ng.a
	ADDON_LIBS += $(base_lib_dir)/libucd.a
	ADDON_LIBS += $(base_lib_dir)/libkissfft-float.a
	ADDON_LIBS += $(base_lib_dir)/libonnxruntime.a
	ADDON_LDFLAGS += -framework Accelerate

vs:
	ADDON_LIBS += C:/Users/yhofmann/Downloads/of_v0.12.1_vs_64_release/addons/ofxSherpaOnnx/libs/sherpa-onnx/lib/Windows_x86_64/sherpa-onnx-c-api.lib
	ADDON_LIBS += C:/Users/yhofmann/Downloads/of_v0.12.1_vs_64_release/addons/ofxSherpaOnnx/libs/sherpa-onnx/lib/Windows_x86_64/sherpa-onnx-cxx-api.lib
	ADDON_LIBS += C:/Users/yhofmann/Downloads/of_v0.12.1_vs_64_release/addons/ofxSherpaOnnx/libs/sherpa-onnx/lib/Windows_x86_64/sherpa-onnx-core.lib
	ADDON_LIBS += C:/Users/yhofmann/Downloads/of_v0.12.1_vs_64_release/addons/ofxSherpaOnnx/libs/sherpa-onnx/lib/Windows_x86_64/kaldi-decoder-core.lib
	ADDON_LIBS += C:/Users/yhofmann/Downloads/of_v0.12.1_vs_64_release/addons/ofxSherpaOnnx/libs/sherpa-onnx/lib/Windows_x86_64/sherpa-onnx-kaldifst-core.lib
	ADDON_LIBS += C:/Users/yhofmann/Downloads/of_v0.12.1_vs_64_release/addons/ofxSherpaOnnx/libs/sherpa-onnx/lib/Windows_x86_64/kaldi-native-fbank-core.lib
	ADDON_LIBS += C:/Users/yhofmann/Downloads/of_v0.12.1_vs_64_release/addons/ofxSherpaOnnx/libs/sherpa-onnx/lib/Windows_x86_64/sherpa-onnx-fst.lib
	ADDON_LIBS += C:/Users/yhofmann/Downloads/of_v0.12.1_vs_64_release/addons/ofxSherpaOnnx/libs/sherpa-onnx/lib/Windows_x86_64/sherpa-onnx-fstfar.lib
	ADDON_LIBS += C:/Users/yhofmann/Downloads/of_v0.12.1_vs_64_release/addons/ofxSherpaOnnx/libs/sherpa-onnx/lib/Windows_x86_64/ssentencepiece_core.lib
	ADDON_LIBS += C:/Users/yhofmann/Downloads/of_v0.12.1_vs_64_release/addons/ofxSherpaOnnx/libs/sherpa-onnx/lib/Windows_x86_64/piper_phonemize.lib
	ADDON_LIBS += C:/Users/yhofmann/Downloads/of_v0.12.1_vs_64_release/addons/ofxSherpaOnnx/libs/sherpa-onnx/lib/Windows_x86_64/espeak-ng.lib
	ADDON_LIBS += C:/Users/yhofmann/Downloads/of_v0.12.1_vs_64_release/addons/ofxSherpaOnnx/libs/sherpa-onnx/lib/Windows_x86_64/ucd.lib
	ADDON_LIBS += C:/Users/yhofmann/Downloads/of_v0.12.1_vs_64_release/addons/ofxSherpaOnnx/libs/sherpa-onnx/lib/Windows_x86_64/kissfft-float.lib
	ADDON_LIBS += C:/Users/yhofmann/Downloads/of_v0.12.1_vs_64_release/addons/ofxSherpaOnnx/libs/sherpa-onnx/lib/Windows_x86_64/onnxruntime.lib

	ADDON_LIBS += ws2_32.lib
	ADDON_LIBS += bcrypt.lib
	ADDON_LIBS += version.lib
