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

		// openFrameworks calls this on the audio thread for each microphone buffer.
		void audioIn(ofSoundBuffer & input) override;

		// Receive changing and completed transcripts from ofxSherpaOnnx.
		void onPartialResultReceived(std::string& result);
		void onFinalResultReceived(std::string& result);

		void drawText(ofTrueTypeFont & font, const std::string & text, float x, float y, const ofColor & color);
		void drawWrappedText(ofTrueTypeFont & font, const std::string & text, float x, float y, float maxWidth, float lineHeight, const ofColor & color);

		// Recognition and microphone objects.
		ofxSherpaOnnx sherpaOnnx;
		ofSoundStream soundStream;
		ofSoundBuffer resampledBuffer;

		// The recognizer expects 16 kHz audio, while the device may use another rate.
		unsigned int modelSampleRate;
		// Result callbacks run on the audio thread, so draw() reads these under a lock.
		std::string currentRecognition;
		std::string finalRecognition;
		std::mutex resultMutex;

		// Optional fonts for the interface; bitmap text is used as a fallback.
		ofTrueTypeFont titleFont;
		ofTrueTypeFont bodyFont;
		ofTrueTypeFont transcriptFont;
		bool fontsReady = false;
};
