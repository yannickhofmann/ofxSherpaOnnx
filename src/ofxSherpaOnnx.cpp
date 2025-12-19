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
#include <cstring> // For memset

ofxSherpaOnnx::ofxSherpaOnnx() {}

ofxSherpaOnnx::~ofxSherpaOnnx() {
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

// ASR (Speech-to-Text)
bool ofxSherpaOnnx::setupASR(const std::string& encoderPath, const std::string& decoderPath, const std::string& joinerPath, const std::string& tokensPath, int sampleRate, const std::string& modelType) {
    SherpaOnnxOnlineRecognizerConfig config{};
    
    config.feat_config.sample_rate = sampleRate;
    config.feat_config.feature_dim = 80;

    config.model_config.num_threads = 1;
    config.model_config.debug = 0;
    config.model_config.provider = "cpu";
    config.model_config.tokens = tokensPath.c_str();
    config.model_config.model_type = modelType.c_str();

    if (modelType == "transducer") {
        config.model_config.transducer.encoder = encoderPath.c_str();
        config.model_config.transducer.decoder = decoderPath.c_str();
        config.model_config.transducer.joiner = joinerPath.c_str();
    } else {
        ofLogError("ofxSherpaOnnx::setupASR") << "Unsupported model type for this setup method: " << modelType;
        return false;
    }

    config.decoding_method = "greedy_search";
    config.max_active_paths = 4;
    config.enable_endpoint = 1;
    config.rule1_min_trailing_silence = 2.4;
    config.rule2_min_trailing_silence = 1.2;
    config.rule3_min_utterance_length = 300;

    if (!ofFile::doesFileExist(tokensPath)) {
        ofLogError("ofxSherpaOnnx::setupASR") << "Tokens file not found: " << tokensPath;
        return false;
    }
    if (!ofFile::doesFileExist(encoderPath) || !ofFile::doesFileExist(decoderPath) || !ofFile::doesFileExist(joinerPath)) {
        ofLogError("ofxSherpaOnnx::setupASR") << "One or more ASR model files not found.";
        return false;
    }

    recognizer = SherpaOnnxCreateOnlineRecognizer(&config);
    if (!recognizer) {
        ofLogError("ofxSherpaOnnx::setupASR") << "Failed to create recognizer.";
        return false;
    }

    stream = SherpaOnnxCreateOnlineStream(recognizer);
    if (!stream) {
        ofLogError("ofxSherpaOnnx::setupASR") << "Failed to create stream.";
        return false;
    }

    ofLogNotice("ofxSherpaOnnx::setupASR") << "SherpaOnnx ASR setup complete.";
    return true;
}

void ofxSherpaOnnx::processASR(const std::vector<float>& audioBuffer) {
    if (!recognizer || !stream) return;
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

void ofxSherpaOnnx::processASR(const ofSoundBuffer& soundBuffer) {
    std::vector<float> audioBuffer(soundBuffer.getBuffer().begin(), soundBuffer.getBuffer().end());
    processASR(audioBuffer);
}

void ofxSherpaOnnx::updateRecognitionResults() {
    if (!recognizer || !stream) return;
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

std::string ofxSherpaOnnx::getCurrentText() { return currentText; }
std::string ofxSherpaOnnx::getFinalText() { return finalText; }

// TTS (Text-to-Speech)
bool ofxSherpaOnnx::setupTTS(const std::string& modelPath, const std::string& lexiconPath, const std::string& tokensPath, float noiseScale, float noiseW, float lengthScale) {
    SherpaOnnxOfflineTtsConfig config;
    memset(&config, 0, sizeof(config));

    ofFile lexiconFile(lexiconPath);
    bool hasLexicon = lexiconFile.exists() && lexiconFile.getSize() > 0;
    std::string lexiconPathToUse = hasLexicon ? lexiconPath : "";
    std::string dataDir;
    if (!hasLexicon) {
        std::string modelDir = ofFilePath::getEnclosingDirectory(modelPath, false);
        dataDir = modelDir + "espeak-ng-data/";
        ofDirectory espeakDir(dataDir);
        if (!espeakDir.exists()) {
            dataDir.clear();
        }
    }

    // Since we are using a Piper VITS model, we configure the `vits` part of the model config.
    config.model.vits.model = modelPath.c_str();
    config.model.vits.lexicon = lexiconPathToUse.c_str();
    config.model.vits.tokens = tokensPath.c_str();
    config.model.vits.data_dir = dataDir.empty() ? "" : dataDir.c_str();
    config.model.vits.noise_scale = noiseScale;
    config.model.vits.noise_scale_w = noiseW;
    config.model.vits.length_scale = lengthScale;

    config.model.num_threads = 1;
    config.model.debug = 1;
    config.model.provider = "cpu";
		
    config.max_num_sentences = 1;


    if (!ofFile::doesFileExist(modelPath) || !ofFile::doesFileExist(tokensPath)) {
        ofLogError("ofxSherpaOnnx::setupTTS") << "One or more required TTS model files not found. Check paths.";
        return false;
    }
    if (!hasLexicon) {
        if (dataDir.empty()) {
            ofLogWarning("ofxSherpaOnnx::setupTTS") << "Lexicon missing or empty and espeak-ng-data not found. TTS init may fail.";
        } else {
            ofLogNotice("ofxSherpaOnnx::setupTTS") << "Lexicon missing or empty; using espeak-ng-data from: " << dataDir;
        }
    }

    ttsSynthesizer = SherpaOnnxCreateOfflineTts(&config);
    if (!ttsSynthesizer) {
        ofLogError("ofxSherpaOnnx::setupTTS") << "Failed to create TTS synthesizer.";
        return false;
    }

    ofLogNotice("ofxSherpaOnnx::setupTTS") << "SherpaOnnx TTS setup complete.";
    return true;
}

bool ofxSherpaOnnx::generateTTS(const std::string& text, std::vector<float>& audioSamples, int& sampleRate) {
    if (!ttsSynthesizer) {
        ofLogError("ofxSherpaOnnx::generateTTS") << "TTS not initialized. Call setupTTS() first.";
        return false;
    }

    float speed = 1.0f;
    int sid = 0; // Speaker ID

    const SherpaOnnxGeneratedAudio* audio = SherpaOnnxOfflineTtsGenerate(ttsSynthesizer, text.c_str(), sid, speed);

    if (!audio || !audio->samples) {
        ofLogError("ofxSherpaOnnx::generateTTS") << "Failed to generate audio for text: " << text;
        if(audio) SherpaOnnxDestroyOfflineTtsGeneratedAudio(audio);
        return false;
    }
    
    audioSamples.assign(audio->samples, audio->samples + audio->n);
    sampleRate = audio->sample_rate;

    SherpaOnnxDestroyOfflineTtsGeneratedAudio(audio);
    
    ofLogNotice("ofxSherpaOnnx::generateTTS") << "Generated " << audioSamples.size() << " samples at " << sampleRate << " Hz.";
    return true;
}
