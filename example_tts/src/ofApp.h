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
#include "ofxSherpaOnnx.h"
#include "ofxGui.h" // For a simple GUI to input text

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

    void onGenerateSpeechButtonPressed();

    ofxSherpaOnnx sherpaOnnx;
    ofSoundPlayer soundPlayer; // To play the generated speech

    ofxPanel gui;
    ofxTextField textInput;
    ofxButton generateSpeechButton;
    
    std::string currentTextToSynthesize;
    bool isSpeaking;
};
