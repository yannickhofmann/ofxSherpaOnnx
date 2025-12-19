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

//--------------------------------------------------------------
void ofApp::setup(){
    ofSetFrameRate(60);
    ofBackground(30, 30, 30);
    ofSetWindowTitle("ofxSherpaOnnx ASR Example");

    // Initialize ofxSherpaOnnx
    // IMPORTANT: You will need to download your own models and tokens.
    // Run the script located at `ofxSherpaOnnx/scripts/download_ASR_model.sh`
    // It will download the necessary model for this example into:
    // `ofxSherpaOnnx/example_asr/bin/data/models/online-zipformer-bilingual-zh-en-2023-02-20/`
    
    // --- Example for an online transducer model (e.g., zipformer) ---
    std::string encoderPath = "models/online-zipformer-bilingual-zh-en-2023-02-20/encoder-epoch-99-avg-1.int8.onnx";
    std::string decoderPath = "models/online-zipformer-bilingual-zh-en-2023-02-20/decoder-epoch-99-avg-1.int8.onnx";
    std::string joinerPath  = "models/online-zipformer-bilingual-zh-en-2023-02-20/joiner-epoch-99-avg-1.int8.onnx";
    std::string tokensPath  = "models/online-zipformer-bilingual-zh-en-2023-02-20/tokens.txt";
    std::string modelType   = "transducer"; // Must match the type of model you downloaded

    modelSampleRate = 16000; // Models are typically trained at 16kHz
    
    // Ensure paths are relative to your data folder
    encoderPath = ofToDataPath(encoderPath, true);
    decoderPath = ofToDataPath(decoderPath, true);
    joinerPath = ofToDataPath(joinerPath, true);
    tokensPath = ofToDataPath(tokensPath, true);

    if (sherpaOnnx.setupASR(encoderPath, decoderPath, joinerPath, tokensPath, modelSampleRate, modelType)) {
        ofLogNotice("ofApp") << "ofxSherpaOnnx setup successful!";
    } else {
        ofLogError("ofApp") << "ofxSherpaOnnx setup failed! Check your model paths and files.";
        // Exit or handle error gracefully
        ofSystemAlertDialog("ofxSherpaOnnx setup failed! Check console for errors and model paths.");
        ofExit();
    }

    // Setup sound stream
    int bufferSize = 512; // Increased buffer size for resampling
    int nInputChannels = 1; // mono input
    int nOutputChannels = 0; // no output (we are only capturing audio)
    int deviceSampleRate = 48000; // Use a standard sample rate your device supports
    
    ofLogNotice("ofApp") << "Available Audio Devices:";
    soundStream.printDeviceList();
    
    // Optional: if you want to set a specific input device (uncomment and change index)
    // auto devices = soundStream.getDeviceList();
    // settings.setInDevice(devices.at(0));

    // Setup the sound stream with the correct sample rate and buffer size
    // We request a standard sample rate from the device, and will resample it down for the model.
    ofSoundStreamSettings settings;
    settings.setInListener(this);
    settings.sampleRate = deviceSampleRate;
    settings.numInputChannels = nInputChannels;
    settings.numOutputChannels = nOutputChannels;
    settings.bufferSize = bufferSize;
    settings.numBuffers = 4;
    soundStream.setup(settings);

    // Register event listeners
    ofAddListener(sherpaOnnx.onPartialResult, this, &ofApp::onPartialResultReceived);
    ofAddListener(sherpaOnnx.onFinalResult, this, &ofApp::onFinalResultReceived);
}

//--------------------------------------------------------------
void ofApp::update(){
    // No continuous update needed here for recognition, processing happens in audioIn
}

//--------------------------------------------------------------
void ofApp::draw(){
    ofSetColor(255);
    ofDrawBitmapString("Say something into the microphone...", 20, 30);
    ofDrawBitmapString("Current Recognition (Partial): " + currentRecognition, 20, 60);
    ofDrawBitmapString("Last Final Recognition: " + finalRecognition, 20, 90);
    
    ofDrawBitmapString("FPS: " + ofToString(ofGetFrameRate()), 20, ofGetHeight() - 20);
}

//--------------------------------------------------------------
void ofApp::audioIn(ofSoundBuffer &input){
    
    // Resample the input buffer if the sample rate does not match the model's required rate.
    if (input.getSampleRate() != modelSampleRate) {
        
        double ratio = (double)modelSampleRate / (double)input.getSampleRate();
        size_t resampledSize = static_cast<size_t>(std::floor(input.getNumFrames() * ratio));
        
        // Ensure the resampled buffer has the correct size and number of channels.
        if (resampledBuffer.getNumFrames() != resampledSize) {
            resampledBuffer.allocate(resampledSize, input.getNumChannels());
        }
        resampledBuffer.setSampleRate(modelSampleRate);

        // Simple linear interpolation for resampling.
        for (size_t i = 0; i < resampledBuffer.getNumFrames(); ++i) {
            double source_index = i / ratio;
            size_t index_0 = static_cast<size_t>(source_index);
            size_t index_1 = std::min(index_0 + 1, input.getNumFrames() - 1);
            double frac = source_index - index_0;

            for (size_t j = 0; j < resampledBuffer.getNumChannels(); ++j) {
                float sample0 = input.getSample(index_0, j);
                float sample1 = input.getSample(index_1, j);
                float resampled_sample = sample0 + (sample1 - sample0) * frac;
                resampledBuffer.getSample(i, j) = resampled_sample;
            }
        }
        
        // Pass the resampled audio buffer to ofxSherpaOnnx for processing.
        sherpaOnnx.processASR(resampledBuffer);

    } else {
        // If sample rates match, process the buffer directly.
        sherpaOnnx.processASR(input);
    }
}

//--------------------------------------------------------------
void ofApp::onPartialResultReceived(std::string& result) {
    ofLogVerbose("ofApp") << "Partial: " << result;
    currentRecognition = result;
}

//--------------------------------------------------------------
void ofApp::onFinalResultReceived(std::string& result) {
    ofLogNotice("ofApp") << "Final: " << result;
    finalRecognition = result;
    currentRecognition = ""; // Clear partial result after a final result is obtained
}

//--------------------------------------------------------------
void ofApp::exit(){
    // Unregister event listeners
    ofRemoveListener(sherpaOnnx.onPartialResult, this, &ofApp::onPartialResultReceived);
    ofRemoveListener(sherpaOnnx.onFinalResult, this, &ofApp::onFinalResultReceived);
    // Stop and close the sound stream
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
