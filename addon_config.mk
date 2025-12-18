# ofxSherpaOnnx addon_config.mk
# Cross-platform configuration for Linux and macOS

meta:
	ADDON_NAME = ofxSherpaOnnx
	ADDON_DESCRIPTION = openFrameworks addon for real-time speech-to-text using sherpa-onnx
	ADDON_AUTHOR = Yannick Hofmann
	ADDON_TAGS = "ai", "speech-to-text", "nlp", "sherpa-onnx", "openframeworks", "ofxaddon"
	ADDON_URL = https://github.com/yannickhofmann/ofxSherpaOnnx

common:
	ADDON_SOURCES = src/ofxSherpaOnnx.cpp
	ADDON_INCLUDES = src
	ADDON_INCLUDES += libs/sherpa-onnx/include

linux64:
	# Libraries in this addon are packaged under a non-standard platform dir
	# (Linux_x86_64), so we declare paths explicitly instead of relying on
	# the automatic parser which looks for $(ABI_LIB_SUBPATH).
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

osx:
	base_lib_dir = libs/sherpa-onnx/lib/Darwin_arm64
	ADDON_LDFLAGS += -L$(base_lib_dir)
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
	ADDON_LDFLAGS += -framework Accelerate
