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

#include "ofxSherpaOnnx.h"
#include <cstring> // std::memset

ofxSherpaOnnx::ofxSherpaOnnx() {}

ofxSherpaOnnx::~ofxSherpaOnnx() {
    // Release every object created by the Sherpa-ONNX C API.
    if (stream) {
        SherpaOnnxDestroyOnlineStream(stream);
    }
    if (recognizer) {
        SherpaOnnxDestroyOnlineRecognizer(recognizer);
    }
    if (ttsSynthesizer) {
        SherpaOnnxDestroyOfflineTts(ttsSynthesizer);
    }
}

//--------------------------------------------------------------
// Configure the recognizer once before sending audio to processASR().
bool ofxSherpaOnnx::setupASR(const std::string& encoderPath, const std::string& decoderPath, const std::string& joinerPath, const std::string& tokensPath, int sampleRate, const std::string& modelType) {
    // Keep path strings as members because the C configuration stores pointers
    // returned by c_str().
    asrEncoderPath = encoderPath;
    asrDecoderPath = decoderPath;
    asrJoinerPath = joinerPath;
    asrTokensPath = tokensPath;
    asrModelType = modelType;

    ofLogNotice("ofxSherpaOnnx::setupASR") << "Initializing ASR with model type: " << asrModelType;

    SherpaOnnxOnlineRecognizerConfig config;
    // C API configuration structs must start with all fields set to zero.
    memset(&config, 0, sizeof(config));

    config.feat_config.sample_rate = sampleRate;
    config.feat_config.feature_dim = 80;

    config.model_config.num_threads = 1;
    config.model_config.debug = 1; // Print Sherpa-ONNX diagnostics during setup.
    config.model_config.provider = "cpu";
    config.model_config.tokens = asrTokensPath.c_str();

    if (asrModelType == "zipformer2") {
        config.model_config.model_type = "zipformer2";
    } else if (asrModelType == "transducer") {
        config.model_config.model_type = "transducer";
    }

    if (asrModelType == "transducer" || asrModelType == "zipformer2") {
        config.model_config.transducer.encoder = asrEncoderPath.c_str();
        config.model_config.transducer.decoder = asrDecoderPath.c_str();
        config.model_config.transducer.joiner = asrJoinerPath.c_str();
    }

    ofLogNotice("ofxSherpaOnnx::setupASR") << "Config set.";

    config.decoding_method = "greedy_search";
    config.max_active_paths = 4;
    config.enable_endpoint = 1;
    config.rule1_min_trailing_silence = 2.4;
    config.rule2_min_trailing_silence = 1.2;
    config.rule3_min_utterance_length = 300;

    // Report missing assets here instead of failing later inside the C API.
    if (!ofFile::doesFileExist(asrTokensPath)) {
        ofLogError("ofxSherpaOnnx::setupASR") << "Tokens file not found: " << asrTokensPath;
        return false;
    }
    if (!ofFile::doesFileExist(asrEncoderPath)) {
        ofLogError("ofxSherpaOnnx::setupASR") << "Encoder not found: " << asrEncoderPath;
        return false;
    }
    if (!ofFile::doesFileExist(asrDecoderPath)) {
        ofLogError("ofxSherpaOnnx::setupASR") << "Decoder not found: " << asrDecoderPath;
        return false;
    }
    if (!ofFile::doesFileExist(asrJoinerPath)) {
        ofLogError("ofxSherpaOnnx::setupASR") << "Joiner not found: " << asrJoinerPath;
        return false;
    }

    ofLogNotice("ofxSherpaOnnx::setupASR") << "Creating recognizer with config...";
    recognizer = SherpaOnnxCreateOnlineRecognizer(&config);
    if (!recognizer) {
        ofLogError("ofxSherpaOnnx::setupASR") << "Failed to create recognizer.";
        return false;
    }

    asrSampleRate = sampleRate;

    ofLogNotice("ofxSherpaOnnx::setupASR") << "Creating stream...";
    stream = SherpaOnnxCreateOnlineStream(recognizer);
    if (!stream) {
        ofLogError("ofxSherpaOnnx::setupASR") << "Failed to create stream.";
        return false;
    }

    ofLogNotice("ofxSherpaOnnx::setupASR") << "SherpaOnnx ASR setup complete.";
    return true;
}

//--------------------------------------------------------------
// Feed one block of mono float audio and let endpoint detection finish phrases.
void ofxSherpaOnnx::processASR(const std::vector<float>& audioBuffer) {
    if (!recognizer || !stream) return;
    if (audioBuffer.empty()) return;

    SherpaOnnxOnlineStreamAcceptWaveform(stream, asrSampleRate, audioBuffer.data(), (int)audioBuffer.size());
    // One input block can make several decoder steps ready.
    while (SherpaOnnxIsOnlineStreamReady(recognizer, stream)) {
        SherpaOnnxDecodeOnlineStream(recognizer, stream);
    }
    updateRecognitionResults();
    if (SherpaOnnxOnlineStreamIsEndpoint(recognizer, stream)) {
        std::string finalToNotify;
        {
            std::lock_guard<std::mutex> lock(mutex);
            if (!currentText.empty()) {
                finalText = currentText;
                finalToNotify = finalText;
                currentText = "";
                lastResultText = "";
            }
        }
        if (!finalToNotify.empty()) {
            // Notify outside the mutex so listeners can safely call back into this class.
            ofNotifyEvent(onFinalResult, finalToNotify, this);
        }
        // Reuse the stream for the next spoken phrase.
        SherpaOnnxOnlineStreamReset(recognizer, stream);
    }
}


//--------------------------------------------------------------
// Decode a block but leave phrase boundaries under the caller's control.
void ofxSherpaOnnx::acceptASRWaveform(const std::vector<float>& audioBuffer) {
    if (!recognizer || !stream) return;
    if (audioBuffer.empty()) return;

    SherpaOnnxOnlineStreamAcceptWaveform(stream, asrSampleRate, audioBuffer.data(), (int)audioBuffer.size());
    while (SherpaOnnxIsOnlineStreamReady(recognizer, stream)) {
        SherpaOnnxDecodeOnlineStream(recognizer, stream);
    }
    updateRecognitionResults();
}

//--------------------------------------------------------------
// Convenience overload for audio received from an openFrameworks sound stream.
void ofxSherpaOnnx::processASR(const ofSoundBuffer& soundBuffer) {
    if (!recognizer || !stream) return;
    if (soundBuffer.size() == 0) return;

    SherpaOnnxOnlineStreamAcceptWaveform(stream, asrSampleRate, soundBuffer.getBuffer().data(), (int)soundBuffer.size());
    while (SherpaOnnxIsOnlineStreamReady(recognizer, stream)) {
        SherpaOnnxDecodeOnlineStream(recognizer, stream);
    }
    updateRecognitionResults();
    if (SherpaOnnxOnlineStreamIsEndpoint(recognizer, stream)) {
        std::string finalToNotify;
        {
            std::lock_guard<std::mutex> lock(mutex);
            if (!currentText.empty()) {
                finalText = currentText;
                finalToNotify = finalText;
                currentText = "";
                lastResultText = "";
            }
        }
        if (!finalToNotify.empty()) {
            ofNotifyEvent(onFinalResult, finalToNotify, this);
        }
        SherpaOnnxOnlineStreamReset(recognizer, stream);
    }
}

//--------------------------------------------------------------
// Copy Sherpa-ONNX text into thread-safe C++ state and emit changed partials.
void ofxSherpaOnnx::updateRecognitionResults() {
    if (!recognizer || !stream) return;
    const SherpaOnnxOnlineRecognizerResult* result = SherpaOnnxGetOnlineStreamResult(recognizer, stream);
    if (result) {
        if (result->text && strlen(result->text) > 0) {
            std::string newText = result->text;
            std::string partialToNotify;
            {
                std::lock_guard<std::mutex> lock(mutex);
                if (newText != lastResultText) {
                    currentText = newText;
                    partialToNotify = currentText;
                    lastResultText = newText;
                }
            }
            if (!partialToNotify.empty()) {
                ofNotifyEvent(onPartialResult, partialToNotify, this);
            }
        }
        SherpaOnnxDestroyOnlineRecognizerResult(result);
    }
}


//--------------------------------------------------------------
// Tell the decoder there is no more audio, then collect its final result.
std::string ofxSherpaOnnx::finishASRAndGetText() {
    if (!recognizer || !stream) return "";

    SherpaOnnxOnlineStreamInputFinished(stream);
    while (SherpaOnnxIsOnlineStreamReady(recognizer, stream)) {
        SherpaOnnxDecodeOnlineStream(recognizer, stream);
    }
    updateRecognitionResults();

    std::string textToReturn;
    {
        std::lock_guard<std::mutex> lock(mutex);
        textToReturn = !currentText.empty() ? currentText : finalText;
        finalText = textToReturn;
        currentText.clear();
        lastResultText.clear();
    }

    resetASRStream();

    if (!textToReturn.empty()) {
        std::string finalToNotify = textToReturn;
        ofNotifyEvent(onFinalResult, finalToNotify, this);
    }

    return textToReturn;
}

//--------------------------------------------------------------
// Create a clean stream so previous decoder state cannot affect a new recording.
void ofxSherpaOnnx::resetASRStream() {
    if (!recognizer) return;

    if (stream) {
        SherpaOnnxDestroyOnlineStream(stream);
        stream = nullptr;
    }

    stream = SherpaOnnxCreateOnlineStream(recognizer);

    std::lock_guard<std::mutex> lock(mutex);
    currentText.clear();
    finalText.clear();
    lastResultText.clear();
}

std::string ofxSherpaOnnx::getCurrentText() {
    std::lock_guard<std::mutex> lock(mutex);
    return currentText;
}
std::string ofxSherpaOnnx::getFinalText() {
    std::lock_guard<std::mutex> lock(mutex);
    return finalText;
}

//--------------------------------------------------------------
// Configure an offline Piper/VITS synthesizer.
bool ofxSherpaOnnx::setupTTS(const std::string& modelPath, const std::string& lexiconPath, const std::string& tokensPath, float noiseScale, float noiseW, float lengthScale) {
    ttsModelPath = modelPath;
    ttsLexiconPath = lexiconPath;
    ttsTokensPath = tokensPath;

    ofLogNotice("ofxSherpaOnnx::setupTTS") << "Initializing TTS...";

    SherpaOnnxOfflineTtsConfig config;
    memset(&config, 0, sizeof(config));

    ofFile lexiconFile(ttsLexiconPath);
    bool hasLexicon = lexiconFile.exists() && lexiconFile.getSize() > 0;
    std::string lexiconPathToUse = hasLexicon ? ttsLexiconPath : "";

    // Models without a lexicon use eSpeak pronunciation data when available.
    std::string dataDir;
    if (!hasLexicon) {
        std::string modelDir = ofFilePath::getEnclosingDirectory(ttsModelPath, false);
        dataDir = modelDir + "espeak-ng-data/";
        ofDirectory espeakDir(dataDir);
        if (!espeakDir.exists()) {
            dataDir.clear();
        }
    }

    // Piper models use the VITS section of Sherpa-ONNX's TTS configuration.
    config.model.vits.model = ttsModelPath.c_str();
    config.model.vits.lexicon = lexiconPathToUse.empty() ? "" : lexiconPathToUse.c_str();
    config.model.vits.tokens = ttsTokensPath.c_str();
    config.model.vits.data_dir = dataDir.empty() ? "" : dataDir.c_str();
    config.model.vits.noise_scale = noiseScale;
    config.model.vits.noise_scale_w = noiseW;
    config.model.vits.length_scale = lengthScale;

    config.model.num_threads = 1;
    config.model.debug = 0;
    config.model.provider = "cpu";

    config.max_num_sentences = 1;

    if (!ofFile::doesFileExist(ttsModelPath) || !ofFile::doesFileExist(ttsTokensPath)) {
        ofLogError("ofxSherpaOnnx::setupTTS") << "One or more required TTS model files not found.";
        return false;
    }

    ofLogNotice("ofxSherpaOnnx::setupTTS") << "Creating TTS synthesizer...";
    ttsSynthesizer = SherpaOnnxCreateOfflineTts(&config);
    if (!ttsSynthesizer) {
        ofLogError("ofxSherpaOnnx::setupTTS") << "Failed to create TTS synthesizer.";
        return false;
    }

    ofLogNotice("ofxSherpaOnnx::setupTTS") << "SherpaOnnx TTS setup complete.";
    return true;
}

//--------------------------------------------------------------
// Synthesize the complete sentence and copy C API memory into a C++ vector.
bool ofxSherpaOnnx::generateTTS(const std::string& text, std::vector<float>& audioSamples, int& sampleRate) {
    if (!ttsSynthesizer) {
        ofLogError("ofxSherpaOnnx::generateTTS") << "TTS not initialized. Call setupTTS() first.";
        return false;
    }

    float speed = 1.0f;
    int sid = 0; // Single-speaker models use speaker zero.

    const SherpaOnnxGeneratedAudio* audio = SherpaOnnxOfflineTtsGenerate(ttsSynthesizer, text.c_str(), sid, speed);

    if (!audio || !audio->samples) {
        ofLogError("ofxSherpaOnnx::generateTTS") << "Failed to generate audio for text: " << text;
        if(audio) SherpaOnnxDestroyOfflineTtsGeneratedAudio(audio);
        return false;
    }

    audioSamples.assign(audio->samples, audio->samples + audio->n);
    sampleRate = audio->sample_rate;

    // The vector now owns a copy, so the C API buffer can be released.
    SherpaOnnxDestroyOfflineTtsGeneratedAudio(audio);

    ofLogNotice("ofxSherpaOnnx::generateTTS") << "Generated " << audioSamples.size() << " samples at " << sampleRate << " Hz.";
    return true;
}
