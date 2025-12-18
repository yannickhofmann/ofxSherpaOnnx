/*
 * ofxSherpaOnnx
 *
 * Copyright (c) 2025 Yannick Hofmann
 * <contact@yannickhofmann.de>
 *
 * BSD Simplified License. 
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#include "ofxSherpaOnnx.h"

ofxSherpaOnnx::ofxSherpaOnnx() : recognizer(nullptr), stream(nullptr) {}

ofxSherpaOnnx::~ofxSherpaOnnx() {
    if (stream) {
        SherpaOnnxDestroyOnlineStream(stream);
    }
    if (recognizer) {
        SherpaOnnxDestroyOnlineRecognizer(recognizer);
    }
}

bool ofxSherpaOnnx::setup(const std::string& encoderPath, const std::string& decoderPath, const std::string& joinerPath, const std::string& tokensPath, int sampleRate, const std::string& modelType) {
    // Zero-initialize the whole config so unused pointers are null.
    SherpaOnnxOnlineRecognizerConfig config{};
    
    // Set up feature configuration
    config.feat_config.sample_rate = sampleRate;
    config.feat_config.feature_dim = 80;

    // Set model configuration
    config.model_config.num_threads = 1;
    config.model_config.debug = 0;
    config.model_config.provider = "cpu";
    config.model_config.tokens = tokensPath.c_str();
    config.model_config.model_type = modelType.c_str();

    // Set model paths based on modelType
    if (modelType == "transducer") {
        config.model_config.transducer.encoder = encoderPath.c_str();
        config.model_config.transducer.decoder = decoderPath.c_str();
        config.model_config.transducer.joiner = joinerPath.c_str();
    } else {
        ofLogError("ofxSherpaOnnx::setup") << "Unsupported model type for this setup method: " << modelType;
        return false;
    }

    // Decoding configuration
    config.decoding_method = "greedy_search";
    config.max_active_paths = 4;

    // Endpoint detection
    config.enable_endpoint = 1;
    config.rule1_min_trailing_silence = 2.4;
    config.rule2_min_trailing_silence = 1.2;
    config.rule3_min_utterance_length = 300;

    // File checks
    if (!ofFile::doesFileExist(tokensPath)) {
        ofLogError("ofxSherpaOnnx::setup") << "Tokens file not found: " << tokensPath;
        return false;
    }
    if (!ofFile::doesFileExist(encoderPath) || !ofFile::doesFileExist(decoderPath) || !ofFile::doesFileExist(joinerPath)) {
        ofLogError("ofxSherpaOnnx::setup") << "One or more model files not found.";
        return false;
    }

    recognizer = SherpaOnnxCreateOnlineRecognizer(&config);
    if (!recognizer) {
        ofLogError("ofxSherpaOnnx::setup") << "Failed to create recognizer. Check model configuration and paths.";
        return false;
    }

    stream = SherpaOnnxCreateOnlineStream(recognizer);
    if (!stream) {
        ofLogError("ofxSherpaOnnx::setup") << "Failed to create stream.";
        return false;
    }

    ofLogNotice("ofxSherpaOnnx::setup") << "SherpaOnnx setup complete.";
    return true;
}

void ofxSherpaOnnx::process(const std::vector<float>& audioBuffer) {
    if (!recognizer || !stream) {
        ofLogError("ofxSherpaOnnx::process") << "Not initialized. Call setup() first.";
        return;
    }

    SherpaOnnxOnlineStreamAcceptWaveform(stream, 16000, audioBuffer.data(), audioBuffer.size());
    
    while (SherpaOnnxIsOnlineStreamReady(recognizer, stream)) {
        SherpaOnnxDecodeOnlineStream(recognizer, stream);
    }
    
    updateRecognitionResults();

    if (SherpaOnnxOnlineStreamIsEndpoint(recognizer, stream)) {
        if (!currentText.empty()) {
            finalText = currentText;
            ofNotifyEvent(onFinalResult, finalText, this);
            currentText = "";
            lastResultText = "";
        }
        SherpaOnnxOnlineStreamReset(recognizer, stream);
    }
}

void ofxSherpaOnnx::process(const ofSoundBuffer& soundBuffer) {
    std::vector<float> audioBuffer(soundBuffer.getBuffer().begin(), soundBuffer.getBuffer().end());
    process(audioBuffer);
}

void ofxSherpaOnnx::updateRecognitionResults() {
    if (!recognizer || !stream) {
        return;
    }

    const SherpaOnnxOnlineRecognizerResult* result = SherpaOnnxGetOnlineStreamResult(recognizer, stream);
    if (result && result->text && strlen(result->text) > 0) {
        std::string newText = result->text;
        if (newText != lastResultText) {
            currentText = newText;
            ofNotifyEvent(onPartialResult, currentText, this);
            lastResultText = newText;
        }
    }
    
    if (result) {
        SherpaOnnxDestroyOnlineRecognizerResult(result);
    }
}

std::string ofxSherpaOnnx::getCurrentText() {
    return currentText;
}

std::string ofxSherpaOnnx::getFinalText() {
    return finalText;
}
