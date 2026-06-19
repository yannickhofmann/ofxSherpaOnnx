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
#include "ofxSherpaOnnx.h"
#include "ofxGui.h" // Provides the text field and generate button.

class ofApp : public ofBaseApp {

public:
    void setup() override;
    void update() override;
    void draw() override;
    void exit() override;

    void keyPressed(int key) override;
    void keyReleased(int key) override;
    void mouseMoved(int x, int y) override;
    void mouseDragged(int x, int y, int button) override;
    void mousePressed(int x, int y, int button) override;
    void mouseReleased(int x, int y, int button) override;
    void mouseEntered(int x, int y) override;
    void mouseExited(int x, int y) override;
    void windowResized(int w, int h) override;
    void dragEvent(ofDragInfo dragInfo) override;
    void gotMessage(ofMessage msg) override;

    // GUI callback and small drawing helpers used by the example.
    void onGenerateSpeechButtonPressed();
    void drawText(ofTrueTypeFont & font, const std::string & text, float x, float y, const ofColor & color);
    void drawWrappedText(ofTrueTypeFont & font, const std::string & text, float x, float y, float maxWidth, float lineHeight, const ofColor & color);

    ofxSherpaOnnx sherpaOnnx;
    ofSoundPlayer soundPlayer; // Plays the temporary WAV created from TTS samples.

    // Controls shown in the standard openFrameworks GUI panel.
    ofxPanel gui;
    ofxTextField textInput;
    ofxButton generateSpeechButton;
    
    // Tracks playback state so repeated clicks cannot overlap generated speech.
    std::string currentTextToSynthesize;
    bool isSpeaking;

    // Custom fonts are optional; draw() falls back to bitmap text if they fail.
    ofTrueTypeFont titleFont;
    ofTrueTypeFont bodyFont;
    ofTrueTypeFont previewFont;
    bool fontsReady = false;
};
