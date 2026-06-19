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
 
 #include "ofApp.h"

#include <cstdlib>

namespace {
// Read the optional language selected by the install scripts or shell.
std::string getSelectedLanguage() {
    const char* value = std::getenv("OFX_SHERPAONNX_LANG");
    if (value == nullptr) {
        return {};
    }
    return value;
}

bool isCompleteAsrModel(const std::string& encoderPath,
                        const std::string& decoderPath,
                        const std::string& joinerPath,
                        const std::string& tokensPath) {
    return ofFile::doesFileExist(ofToDataPath(encoderPath, true)) &&
           ofFile::doesFileExist(ofToDataPath(decoderPath, true)) &&
           ofFile::doesFileExist(ofToDataPath(joinerPath, true)) &&
           ofFile::doesFileExist(ofToDataPath(tokensPath, true));
}
}

//--------------------------------------------------------------
void ofApp::setup(){
    ofSetFrameRate(60);
    ofBackground(30, 30, 30);
    ofSetWindowTitle("ofxSherpaOnnx ASR Example");

    fontsReady = titleFont.load("fonts/verdana.ttf", 18, true, true, true) &&
        bodyFont.load("fonts/verdana.ttf", 13, true, true, true) &&
        transcriptFont.load("fonts/verdana.ttf", 24, true, true, true);
    if (!fontsReady) {
        ofLogWarning("ofApp") << "Could not load fonts/verdana.ttf; falling back to bitmap text.";
    }

    // Select German by default. Set OFX_SHERPAONNX_LANG=en to prefer English.
    // The download scripts place the model files in bin/data/models.
    
    const std::string requestedLang = getSelectedLanguage();
    const bool preferEnglish = requestedLang == "en";

    const std::string germanFolder = "models/sherpa-onnx-streaming-zipformer-de-kroko-2025-08-06/";
    const std::string englishFolder = "models/online-zipformer-bilingual-zh-en-2023-02-20/";

    std::string modelFolder = preferEnglish ? englishFolder : germanFolder;
    std::string encoderPath = preferEnglish ?
        modelFolder + "encoder-epoch-99-avg-1.int8.onnx" :
        modelFolder + "encoder.onnx";
    std::string decoderPath = preferEnglish ?
        modelFolder + "decoder-epoch-99-avg-1.int8.onnx" :
        modelFolder + "decoder.onnx";
    std::string joinerPath = preferEnglish ?
        modelFolder + "joiner-epoch-99-avg-1.int8.onnx" :
        modelFolder + "joiner.onnx";
    std::string tokensPath = modelFolder + "tokens.txt";
    std::string modelType = preferEnglish ? "transducer" : "zipformer2";

    // Fall back to the other installed language instead of stopping immediately.
    if (!isCompleteAsrModel(encoderPath, decoderPath, joinerPath, tokensPath)) {
        const bool germanAvailable = isCompleteAsrModel(
            germanFolder + "encoder.onnx",
            germanFolder + "decoder.onnx",
            germanFolder + "joiner.onnx",
            germanFolder + "tokens.txt");
        const bool englishAvailable = isCompleteAsrModel(
            englishFolder + "encoder-epoch-99-avg-1.int8.onnx",
            englishFolder + "decoder-epoch-99-avg-1.int8.onnx",
            englishFolder + "joiner-epoch-99-avg-1.int8.onnx",
            englishFolder + "tokens.txt");

        if (preferEnglish && germanAvailable) {
            ofLogNotice("ofApp") << "English model not found, falling back to German model.";
            modelFolder = germanFolder;
            encoderPath = modelFolder + "encoder.onnx";
            decoderPath = modelFolder + "decoder.onnx";
            joinerPath = modelFolder + "joiner.onnx";
            modelType = "zipformer2";
        } else if (!preferEnglish && englishAvailable) {
            ofLogNotice("ofApp") << "German model not found, falling back to English model.";
            modelFolder = englishFolder;
            encoderPath = modelFolder + "encoder-epoch-99-avg-1.int8.onnx";
            decoderPath = modelFolder + "decoder-epoch-99-avg-1.int8.onnx";
            joinerPath = modelFolder + "joiner-epoch-99-avg-1.int8.onnx";
            modelType = "transducer";
        } else {
            ofLogError("ofApp") << "No complete ASR model found. Run the English or German download script first.";
            ofSystemAlertDialog("No complete ASR model found. Run the English or German download script first.");
            ofExit();
            return;
        }
    }

    modelSampleRate = 16000; // These models expect mono audio at 16 kHz.
    
    // Convert data-relative paths to absolute paths for the Sherpa-ONNX C API.
    encoderPath = ofToDataPath(encoderPath, true);
    decoderPath = ofToDataPath(decoderPath, true);
    joinerPath = ofToDataPath(joinerPath, true);
    tokensPath = ofToDataPath(tokensPath, true);

    if (sherpaOnnx.setupASR(encoderPath, decoderPath, joinerPath, tokensPath, modelSampleRate, modelType)) {
        ofLogNotice("ofApp") << "ofxSherpaOnnx setup successful!";
    } else {
        ofLogError("ofApp") << "ofxSherpaOnnx setup failed! Check your model paths and files.";
        ofSystemAlertDialog("ofxSherpaOnnx setup failed! Check console for errors and model paths.");
        ofExit();
    }

    // Capture mono microphone input only. A common device rate is used here and
    // audioIn() converts it to the model rate when necessary.
    int bufferSize = 512;
    int nInputChannels = 1;
    int nOutputChannels = 0;
    int deviceSampleRate = 48000;
    
    ofLogNotice("ofApp") << "Available Audio Devices:";
    soundStream.printDeviceList();
    
    // Optional: if you want to set a specific input device (uncomment and change index)
    // auto devices = soundStream.getDeviceList();
    // settings.setInDevice(devices.at(0));

    // Register this ofApp as the receiver for microphone buffers.
    ofSoundStreamSettings settings;
    settings.setInListener(this);
    settings.sampleRate = deviceSampleRate;
    settings.numInputChannels = nInputChannels;
    settings.numOutputChannels = nOutputChannels;
    settings.bufferSize = bufferSize;
    settings.numBuffers = 4;
    soundStream.setup(settings);

    // Recognition events update the text displayed by draw().
    ofAddListener(sherpaOnnx.onPartialResult, this, &ofApp::onPartialResultReceived);
    ofAddListener(sherpaOnnx.onFinalResult, this, &ofApp::onFinalResultReceived);
}

//--------------------------------------------------------------
void ofApp::update(){
    // Recognition is driven by audioIn(), so no per-frame work is needed.
}

//--------------------------------------------------------------
void ofApp::draw(){
    ofBackground(30, 30, 30);

    std::string partial, final;
    {
        // Copy shared results quickly, then draw without holding the mutex.
        std::lock_guard<std::mutex> lock(resultMutex);
        partial = currentRecognition;
        final = finalRecognition;
    }

    const float margin = 24.0f;
    const float maxWidth = ofGetWidth() - margin * 2.0f;

    if (fontsReady) {
        drawText(titleFont, "Say something into the microphone...", margin, 34, ofColor(255));
        drawText(bodyFont, "Current Recognition (Partial):", margin, 76, ofColor(170, 210, 255));
        drawWrappedText(transcriptFont, partial.empty() ? "..." : partial, margin, 112, maxWidth, 34.0f, ofColor(255, 236, 198));
        drawText(bodyFont, "Last Final Recognition:", margin, 210, ofColor(170, 210, 255));
        drawWrappedText(transcriptFont, final.empty() ? "..." : final, margin, 246, maxWidth, 34.0f, ofColor(255, 236, 198));
        drawText(bodyFont, "FPS: " + ofToString(ofGetFrameRate()), margin, ofGetHeight() - 20, ofColor(255, 255, 255, 170));
    } else {
        ofSetColor(255);
        ofDrawBitmapString("Say something into the microphone...", margin, 30);
        ofDrawBitmapString("Current Recognition (Partial): " + partial, margin, 60);
        ofDrawBitmapString("Last Final Recognition: " + final, margin, 90);
        ofDrawBitmapString("FPS: " + ofToString(ofGetFrameRate()), margin, ofGetHeight() - 20);
    }
}


//--------------------------------------------------------------
void ofApp::drawText(ofTrueTypeFont & font, const std::string & text, float x, float y, const ofColor & color) {
    ofPushStyle();
    ofSetColor(color);
    font.drawString(text, x, y);
    ofPopStyle();
}

//--------------------------------------------------------------
void ofApp::drawWrappedText(ofTrueTypeFont & font, const std::string & text, float x, float y, float maxWidth, float lineHeight, const ofColor & color) {
    // Build each line word by word until adding another word exceeds maxWidth.
    std::vector<std::string> words = ofSplitString(text, " ", true, true);
    std::string line;
    float drawY = y;

    ofPushStyle();
    ofSetColor(color);
    for (const auto & word : words) {
        const std::string candidate = line.empty() ? word : line + " " + word;
        if (!line.empty() && font.stringWidth(candidate) > maxWidth) {
            font.drawString(line, x, drawY);
            drawY += lineHeight;
            line = word;
        } else {
            line = candidate;
        }
    }
    if (!line.empty()) {
        font.drawString(line, x, drawY);
    }
    ofPopStyle();
}

//--------------------------------------------------------------
void ofApp::audioIn(ofSoundBuffer &input){
    
    // Audio callbacks run on a separate thread. Keep this path short and allocation-light.
    // Resample only when the device rate differs from the model's required rate.
    if (input.getSampleRate() != modelSampleRate) {
        float speed = (float)input.getSampleRate() / (float)modelSampleRate;
        size_t targetFrames = static_cast<size_t>(std::ceil(input.getNumFrames() / speed));
        
        input.resampleTo(resampledBuffer, 0, targetFrames, speed);
        resampledBuffer.setSampleRate(modelSampleRate);
        
        sherpaOnnx.processASR(resampledBuffer);

    } else {
        // Avoid an unnecessary copy when both sample rates already match.
        sherpaOnnx.processASR(input);
    }
}

//--------------------------------------------------------------
void ofApp::onPartialResultReceived(std::string& result) {
    ofLogVerbose("ofApp") << "Partial: " << result;
    std::lock_guard<std::mutex> lock(resultMutex);
    currentRecognition = result;
}

//--------------------------------------------------------------
void ofApp::onFinalResultReceived(std::string& result) {
    ofLogNotice("ofApp") << "Final: " << result;
    std::lock_guard<std::mutex> lock(resultMutex);
    finalRecognition = result;
    currentRecognition = ""; // The completed text replaces the partial result.
}

//--------------------------------------------------------------
void ofApp::exit(){
    // Stop callbacks before closing the audio device and destroying the app.
    ofRemoveListener(sherpaOnnx.onPartialResult, this, &ofApp::onPartialResultReceived);
    ofRemoveListener(sherpaOnnx.onFinalResult, this, &ofApp::onFinalResultReceived);
    soundStream.stop();
    soundStream.close();
}

//--------------------------------------------------------------
void ofApp::keyPressed(int key){

}

//--------------------------------------------------------------
void ofApp::keyReleased(int key){

}

//--------------------------------------------------------------
void ofApp::mouseMoved(int x, int y ){

}

//--------------------------------------------------------------
void ofApp::mouseDragged(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mousePressed(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mouseReleased(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mouseEntered(int x, int y){

}

//--------------------------------------------------------------
void ofApp::mouseExited(int x, int y){

}

//--------------------------------------------------------------
void ofApp::windowResized(int w, int h){

}

//--------------------------------------------------------------
void ofApp::dragEvent(ofDragInfo dragInfo){ 

}

//--------------------------------------------------------------
void ofApp::gotMessage(ofMessage msg){

}
