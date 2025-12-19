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

    // ASR (Speech-to-Text)
    bool setupASR(const std::string& encoderPath, const std::string& decoderPath, const std::string& joinerPath, const std::string& tokensPath, int sampleRate, const std::string& modelType);
    void processASR(const std::vector<float>& audioBuffer);
    void processASR(const ofSoundBuffer& soundBuffer);
    std::string getCurrentText();
    std::string getFinalText();
    ofEvent<std::string> onPartialResult;
    ofEvent<std::string> onFinalResult;

    // TTS (Text-to-Speech)
    bool setupTTS(const std::string& modelPath, const std::string& lexiconPath, const std::string& tokensPath, float noiseScale, float noiseW, float lengthScale);
    bool generateTTS(const std::string& text, std::vector<float>& audioSamples, int& sampleRate);

private:
    // ASR members
    const SherpaOnnxOnlineRecognizer* recognizer = nullptr;
    const SherpaOnnxOnlineStream* stream = nullptr;
    std::string currentText;
    std::string finalText;
    void updateRecognitionResults();
    std::string lastResultText; // To track changes and fire events

    // TTS members
    const SherpaOnnxOfflineTts* ttsSynthesizer = nullptr;
};
