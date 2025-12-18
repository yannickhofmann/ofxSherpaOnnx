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
 
#pragma once

#include "ofMain.h"
#include "sherpa-onnx/c-api/c-api.h"

class ofxSherpaOnnx {
public:
    ofxSherpaOnnx();
    ~ofxSherpaOnnx();

    // Setup and configuration
    // modelType can be "transducer", "zipformer", "wenet", etc.
    // Ensure you provide the correct paths for the model architecture you are using.
    // For transducer: encoder, decoder, joiner, tokens
    // For others: model, tokens
    bool setup(const std::string& encoderPath, const std::string& decoderPath, const std::string& joinerPath, const std::string& tokensPath, int sampleRate, const std::string& modelType);

    // Processing audio
    void process(const std::vector<float>& audioBuffer);
    void process(const ofSoundBuffer& soundBuffer);

    // Get results (optional, events are preferred for continuous updates)
    std::string getCurrentText();
    std::string getFinalText();
    
    // Events
    ofEvent<std::string> onPartialResult;
    ofEvent<std::string> onFinalResult;

private:
    const SherpaOnnxOnlineRecognizer* recognizer;
    const SherpaOnnxOnlineStream* stream;
    
    std::string currentText;
    std::string finalText;

    void updateRecognitionResults();
    std::string lastResultText; // To track changes and fire events
};
