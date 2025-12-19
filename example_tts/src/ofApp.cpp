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
 
 #include "ofApp.h"
 #include "ofxSoundFile.h"

//--------------------------------------------------------------
void ofApp::setup(){
    ofSetFrameRate(60);
    ofBackground(30, 30, 30);
    ofSetWindowTitle("ofxSherpaOnnx TTS Example");

    // Initialize ofxSherpaOnnx for TTS
    // IMPORTANT: You will need to download your own models.
    // Run the script located at `ofxSherpaOnnx/scripts/download_TTS_model.sh`
    // It will download the necessary model for this example into:
    // `ofxSherpaOnnx/example_tts/bin/data/models/vits-piper-en_US-amy-low/`

    std::string modelPath = "models/vits-piper-en_US-amy-low/model.onnx";
    std::string lexiconPath = "models/vits-piper-en_US-amy-low/lexicon.txt";
    std::string tokensPath = "models/vits-piper-en_US-amy-low/tokens.txt";
    float noiseScale = 0.667f;
    float noiseW = 0.8f;
    float lengthScale = 1.0f;
    
    // Ensure paths are relative to your data folder
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
    
    // Setup GUI
    gui.setup("TTS Controls");
    gui.setPosition(20, 50);
    textInput.setup("Text", "Hello, this is a test of text to speech.");
    gui.add(&textInput); // Add address of the textInput
    generateSpeechButton.setup("Generate Speech");
    gui.add(&generateSpeechButton); // Add address of the button
    generateSpeechButton.addListener(this, &ofApp::onGenerateSpeechButtonPressed);
    
    isSpeaking = false;
}

//--------------------------------------------------------------
void ofApp::update(){
    // Check if the sound player is done speaking
    if (isSpeaking && !soundPlayer.isPlaying()) {
        isSpeaking = false;
        ofLogNotice("ofApp") << "Finished speaking.";
    }
}

//--------------------------------------------------------------
void ofApp::draw(){
    ofSetColor(255);
    ofDrawBitmapString("Enter text and press 'Generate Speech'", 20, 30);
    
    gui.draw();
    
    if (isSpeaking) {
        ofDrawBitmapString("Speaking...", 20, gui.getPosition().y + gui.getHeight() + 20);
    }
    
    ofDrawBitmapString("FPS: " + ofToString(ofGetFrameRate()), 20, ofGetHeight() - 20);
}

//--------------------------------------------------------------
void ofApp::onGenerateSpeechButtonPressed(){
    if (isSpeaking) {
        ofLogWarning("ofApp") << "Already speaking, please wait.";
        return;
    }
    
    currentTextToSynthesize = textInput;
    ofLogNotice("ofApp") << "Generating speech for: " << currentTextToSynthesize;

    // Generate TTS audio
    std::vector<float> audioSamples;
    int sampleRate;
    
    if (sherpaOnnx.generateTTS(currentTextToSynthesize, audioSamples, sampleRate)) {
        ofLogNotice("ofApp") << "Speech generated successfully! Sample rate: " << sampleRate << ", Samples: " << audioSamples.size();
        
        // Convert float samples to an ofSoundBuffer
        ofSoundBuffer buffer;
        buffer.setSampleRate(sampleRate);
        buffer.setNumChannels(1); // TTS typically generates mono audio
        buffer.allocate(audioSamples.size(), 1); // Corrected allocate call
        
        // Copy samples to ofSoundBuffer
        for (size_t i = 0; i < audioSamples.size(); ++i) {
            buffer.getSample(i, 0) = audioSamples[i];
        }

        // Save the buffer to a temporary WAV file and then load it with ofSoundPlayer
#ifdef TARGET_OSX
        std::string tempWavPath = "temp_tts.wav";
#else
        std::string tempWavPath = ofToDataPath("temp_tts.wav", true);
#endif
        if (!ofxSaveSound(buffer, tempWavPath)) {
            ofLogError("ofApp") << "Failed to save TTS audio to: " << tempWavPath;
            return;
        }
        
        // Play the audio
        soundPlayer.load(tempWavPath); // Corrected load call
        soundPlayer.play();
        isSpeaking = true;

    } else {
        ofLogError("ofApp") << "Failed to generate speech.";
    }
}

//--------------------------------------------------------------
void ofApp::exit(){
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
