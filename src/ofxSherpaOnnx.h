/*
 * ofxSherpaOnnx
 *
 * Copyright (c) 2026 Yannick Hofmann
 * <contact@yannickhofmann.de>
 *
 * BSD Simplified License. 
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */
 
#pragma once

#include "ofMain.h"
#include "sherpa-onnx/c-api/c-api.h"
#include <mutex>

class ofxSherpaOnnx {
public:
    ofxSherpaOnnx();
    ~ofxSherpaOnnx();

    // Set up streaming automatic speech recognition (ASR).
    // Model paths must point to valid files and incoming audio must use sampleRate.
    bool setupASR(const std::string& encoderPath, const std::string& decoderPath, const std::string& joinerPath, const std::string& tokensPath, int sampleRate, const std::string& modelType);
    void processASR(const std::vector<float>& audioBuffer);
    void processASR(const ofSoundBuffer& soundBuffer);
    // Decode audio without automatic endpoint handling. This is useful when an
    // application records first and decides itself when the utterance ends.
    void acceptASRWaveform(const std::vector<float>& audioBuffer);
    // Flush pending audio, return the completed transcript and start a new stream.
    std::string finishASRAndGetText();
    void resetASRStream();
    std::string getCurrentText();
    std::string getFinalText();
    ofEvent<std::string> onPartialResult; // Fires whenever the partial text changes.
    ofEvent<std::string> onFinalResult;   // Fires when an utterance is complete.

    // Set up text-to-speech (TTS) and generate mono floating-point samples.
    bool setupTTS(const std::string& modelPath, const std::string& lexiconPath, const std::string& tokensPath, float noiseScale, float noiseW, float lengthScale);
    bool generateTTS(const std::string& text, std::vector<float>& audioSamples, int& sampleRate);

private:
    // Sherpa-ONNX owns these objects; the destructor releases them in reverse order.
    const SherpaOnnxOnlineRecognizer* recognizer = nullptr;
    const SherpaOnnxOnlineStream* stream = nullptr;
    int asrSampleRate = 16000;
    std::string currentText;
    std::string finalText;
    void updateRecognitionResults();
    std::string lastResultText; // Prevents duplicate partial-result events.

    // The C API stores const char pointers while creating its model objects, so
    // member strings keep the underlying path text alive during setup.
    std::string asrEncoderPath, asrDecoderPath, asrJoinerPath, asrTokensPath, asrModelType;

    // TTS uses a separate offline synthesizer.
    const SherpaOnnxOfflineTts* ttsSynthesizer = nullptr;
    std::string ttsModelPath, ttsLexiconPath, ttsTokensPath;

    // Audio callbacks and the main application thread may access results together.
    std::mutex mutex;
};
