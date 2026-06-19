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

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <fstream>

namespace {
// Read the optional language selected by the install scripts or shell.
std::string getSelectedLanguage() {
    const char* value = std::getenv("OFX_SHERPAONNX_LANG");
    if (value == nullptr) {
        return {};
    }
    return value;
}

bool isCompleteTtsModel(const std::string& modelPath,
                        const std::string& tokensPath) {
    return ofFile::doesFileExist(ofToDataPath(modelPath, true)) &&
           ofFile::doesFileExist(ofToDataPath(tokensPath, true));
}

// WAV headers store integer values in little-endian byte order.
void writeU16(std::ofstream & out, uint16_t value) {
    out.put(static_cast<char>(value & 0xff));
    out.put(static_cast<char>((value >> 8) & 0xff));
}

void writeU32(std::ofstream & out, uint32_t value) {
    out.put(static_cast<char>(value & 0xff));
    out.put(static_cast<char>((value >> 8) & 0xff));
    out.put(static_cast<char>((value >> 16) & 0xff));
    out.put(static_cast<char>((value >> 24) & 0xff));
}

bool savePcm16Wav(const std::string & path, const std::vector<float> & samples, int sampleRate) {
    if (samples.empty() || sampleRate <= 0) {
        return false;
    }

    std::ofstream out(path, std::ios::binary);
    if (!out.is_open()) {
        ofLogError("ofApp") << "Could not open WAV file for writing: " << path;
        return false;
    }

    // ofSoundPlayer reliably handles a standard mono, 16-bit PCM WAV.
    const uint16_t channels = 1;
    const uint16_t bitsPerSample = 16;
    const uint16_t blockAlign = channels * bitsPerSample / 8;
    const uint32_t byteRate = static_cast<uint32_t>(sampleRate) * blockAlign;
    const uint32_t dataSize = static_cast<uint32_t>(samples.size() * sizeof(int16_t));

    out.write("RIFF", 4);
    writeU32(out, 36 + dataSize);
    out.write("WAVE", 4);
    out.write("fmt ", 4);
    writeU32(out, 16);
    writeU16(out, 1);
    writeU16(out, channels);
    writeU32(out, static_cast<uint32_t>(sampleRate));
    writeU32(out, byteRate);
    writeU16(out, blockAlign);
    writeU16(out, bitsPerSample);
    out.write("data", 4);
    writeU32(out, dataSize);

    // TTS returns normalized floats; PCM needs clipped signed 16-bit values.
    for (float sample : samples) {
        const float clipped = std::max(-1.0f, std::min(1.0f, sample));
        const int16_t pcm = static_cast<int16_t>(std::lrint(clipped * 32767.0f));
        writeU16(out, static_cast<uint16_t>(pcm));
    }

    return out.good();
}

}

//--------------------------------------------------------------
void ofApp::setup(){
    ofSetFrameRate(60);
    ofBackground(30, 30, 30);
    ofSetWindowTitle("ofxSherpaOnnx TTS Example");

    fontsReady = titleFont.load("fonts/verdana.ttf", 18, true, true, true) &&
        bodyFont.load("fonts/verdana.ttf", 13, true, true, true) &&
        previewFont.load("fonts/verdana.ttf", 20, true, true, true);
    if (fontsReady) {
        ofxGuiSetFont("fonts/verdana.ttf", 12, true, true);
    } else {
        ofLogWarning("ofApp") << "Could not load fonts/verdana.ttf; falling back to default GUI text.";
    }

    // Select German by default. Set OFX_SHERPAONNX_LANG=en to prefer English.
    // The download scripts place both supported models in bin/data/models.

    const std::string requestedLang = getSelectedLanguage();
    const bool preferEnglish = requestedLang == "en";

    const std::string germanFolder = "models/vits-piper-de_DE-thorsten-low/";
    const std::string englishFolder = "models/vits-piper-en_US-amy-low/";

    std::string modelFolder = preferEnglish ? englishFolder : germanFolder;
    std::string modelPath = modelFolder + "model.onnx";
    std::string tokensPath = modelFolder + "tokens.txt";
    std::string lexiconPath = modelFolder + "lexicon.txt"; // An empty lexicon is supported.

    std::string defaultText = preferEnglish
        ? "Hello, this is a test of text to speech in openFrameworks."
        : "Hallo, ich bin ein Test der Sprachsynthese mit Umlauten: ä ö ü Ä Ö Ü ß.";

    // Fall back to the other installed language instead of stopping immediately.
    if (!isCompleteTtsModel(modelPath, tokensPath)) {
        const bool germanAvailable = isCompleteTtsModel(
            germanFolder + "model.onnx",
            germanFolder + "tokens.txt");
        const bool englishAvailable = isCompleteTtsModel(
            englishFolder + "model.onnx",
            englishFolder + "tokens.txt");

        if (preferEnglish && germanAvailable) {
            ofLogNotice("ofApp") << "English TTS model not found, falling back to German model.";
            modelFolder = germanFolder;
            modelPath = modelFolder + "model.onnx";
            tokensPath = modelFolder + "tokens.txt";
            lexiconPath = modelFolder + "lexicon.txt";
            defaultText = "Hallo, ich bin ein Test der Sprachsynthese mit Umlauten: ä ö ü Ä Ö Ü ß.";
        } else if (!preferEnglish && englishAvailable) {
            ofLogNotice("ofApp") << "German TTS model not found, falling back to English model.";
            modelFolder = englishFolder;
            modelPath = modelFolder + "model.onnx";
            tokensPath = modelFolder + "tokens.txt";
            lexiconPath = modelFolder + "lexicon.txt";
            defaultText = "Hello, this is a test of text to speech in openFrameworks.";
        } else {
            ofLogError("ofApp") << "No complete TTS model found. Run the English or German download script first.";
            ofSystemAlertDialog("No complete TTS model found. Run the English or German download script first.");
            ofExit();
            return;
        }
    }

    float noiseScale = 0.667f;
    float noiseW = 0.8f;
    float lengthScale = 1.0f;
    
    // Convert data-relative paths to absolute paths for the Sherpa-ONNX C API.
    modelPath = ofToDataPath(modelPath, true);
    lexiconPath = ofToDataPath(lexiconPath, true);
    tokensPath = ofToDataPath(tokensPath, true);

    if (sherpaOnnx.setupTTS(modelPath, lexiconPath, tokensPath, noiseScale, noiseW, lengthScale)) {
        ofLogNotice("ofApp") << "ofxSherpaOnnx TTS setup successful!";
    } else {
        ofLogError("ofApp") << "ofxSherpaOnnx TTS setup failed! Check your model paths and files.";
        ofSystemAlertDialog("ofxSherpaOnnx TTS setup failed! Check console for errors and model paths.");
        ofExit();
    }
    
    // Build the controls and connect the button to its callback.
    gui.setup("TTS Controls");
    gui.setPosition(20, 50);
    textInput.setup("Text", defaultText);
    gui.add(&textInput);
    generateSpeechButton.setup("Generate Speech");
    gui.add(&generateSpeechButton);
    generateSpeechButton.addListener(this, &ofApp::onGenerateSpeechButtonPressed);
    
    isSpeaking = false;
}

//--------------------------------------------------------------
void ofApp::update(){
    // Return to the ready state when asynchronous WAV playback finishes.
    if (isSpeaking && !soundPlayer.isPlaying()) {
        isSpeaking = false;
        ofLogNotice("ofApp") << "Finished speaking.";
    }
}

//--------------------------------------------------------------
void ofApp::draw(){
    ofBackground(30, 30, 30);

    if (fontsReady) {
        drawText(titleFont, "Enter text and press 'Generate Speech'", 20, 30, ofColor(255));
    } else {
        ofSetColor(255);
        ofDrawBitmapString("Enter text and press 'Generate Speech'", 20, 30);
    }

    gui.draw();

    // Place status and preview text directly below the GUI panel.
    const float statusY = gui.getPosition().y + gui.getHeight() + 28;
    if (fontsReady) {
        drawText(bodyFont, isSpeaking ? "Speaking..." : "Ready", 20, statusY, isSpeaking ? ofColor(255, 236, 198) : ofColor(170, 210, 255));
        drawText(bodyFont, "Text preview:", 20, statusY + 38, ofColor(170, 210, 255));
        drawWrappedText(previewFont, static_cast<std::string>(textInput), 20, statusY + 74, ofGetWidth() - 40, 30.0f, ofColor(255, 236, 198));
        drawText(bodyFont, "FPS: " + ofToString(ofGetFrameRate()), 20, ofGetHeight() - 20, ofColor(255, 255, 255, 170));
    } else {
        if (isSpeaking) {
            ofDrawBitmapString("Speaking...", 20, statusY);
        }
        ofDrawBitmapString("FPS: " + ofToString(ofGetFrameRate()), 20, ofGetHeight() - 20);
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
void ofApp::onGenerateSpeechButtonPressed(){
    if (isSpeaking) {
        ofLogWarning("ofApp") << "Already speaking, please wait.";
        return;
    }
    
    currentTextToSynthesize = textInput;
    ofLogNotice("ofApp") << "Generating speech for: " << currentTextToSynthesize;

    // Sherpa-ONNX returns normalized mono samples and the model's sample rate.
    std::vector<float> audioSamples;
    int sampleRate;
    
    if (sherpaOnnx.generateTTS(currentTextToSynthesize, audioSamples, sampleRate)) {
        ofLogNotice("ofApp") << "Speech generated successfully! Sample rate: " << sampleRate << ", Samples: " << audioSamples.size();
        
        // A standard PCM WAV avoids noisy float-WAV playback on some Linux/FMOD paths.
        std::string tempWavPath = ofToDataPath("temp_tts.wav", true);
        if (!savePcm16Wav(tempWavPath, audioSamples, sampleRate)) {
            ofLogError("ofApp") << "Failed to save TTS audio to: " << tempWavPath;
            return;
        }

        // ofSoundPlayer loads files, so play the temporary WAV rather than raw samples.
        soundPlayer.load(tempWavPath);
        soundPlayer.play();
        isSpeaking = true;

    } else {
        ofLogError("ofApp") << "Failed to generate speech.";
    }
}

//--------------------------------------------------------------
void ofApp::exit(){
    // Remove callbacks before their owning objects are destroyed.
    generateSpeechButton.removeListener(this, &ofApp::onGenerateSpeechButtonPressed);
    soundPlayer.stop();
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
