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

#include <atomic>
#include <mutex>
#include <thread>
#include <vector>

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

	void audioIn(ofSoundBuffer & input) override;

	// Sherpa-ONNX sends partial and final text through these event callbacks.
	void onPartialResultReceived(std::string & result);
	void onFinalResultReceived(std::string & result);

private:
	// Setup is split into model and microphone stages to keep each step readable.
	void setupModel();
	void setupAudioInput();
	void startRecording();
	void stopRecordingAndTranscribe();
	void transcribeRecordedAudio(std::vector<float> samples, unsigned int channels, unsigned int sourceSampleRate);
	std::vector<float> makeMono16kSamples(const std::vector<float> & samples, unsigned int channels, unsigned int sourceSampleRate);
	std::string stateLabel() const;
	void drawText(ofTrueTypeFont & font, const std::string & text, float x, float y, const ofColor & color);
	void drawWrappedText(ofTrueTypeFont & font, const std::string & text, float x, float y, float maxWidth, float lineHeight, const ofColor & color);

	// Recognition and microphone objects.
	ofxSherpaOnnx sherpaOnnx;
	ofSoundStream soundStream;
	ofSoundBuffer resampledBuffer;

	// Device audio is converted to the mono 16 kHz format expected by the model.
	unsigned int modelSampleRate = 16000;
	unsigned int inputSampleRate = 48000;
	unsigned int inputChannels = 1;
	// A short grace period captures the end of speech after SPACE is released.
	float stopGraceSeconds = 0.30f;
	// Decoder tail is synthetic silence that helps the model finish its last word.
	float decoderTailSeconds = 2.50f;
	float stopRequestedAt = 0.0f;
	bool stopPending = false;

	// These flags are shared by the main, audio and transcription threads.
	std::atomic<bool> modelReady { false };
	std::atomic<bool> audioReady { false };
	std::atomic<bool> isRecording { false };
	std::atomic<bool> isTranscribing { false };
	std::atomic<bool> shouldExit { false };

	// audioIn() appends here while the worker thread later consumes a copy.
	std::vector<float> recordedSamples;
	std::mutex audioMutex;

	// Recognition runs off the main thread so drawing remains responsive.
	std::thread transcriptionThread;

	// Text and timing values are protected because events can update them concurrently.
	std::string currentRecognition;
	std::string finalRecognition;
	std::string statusText = "Press SPACE to start recording";
	float lastRecordingSeconds = 0.0f;
	float lastTranscribeMillis = 0.0f;
	std::mutex resultMutex;

	// Optional fonts for the interface; bitmap text is used as a fallback.
	ofTrueTypeFont titleFont;
	ofTrueTypeFont bodyFont;
	ofTrueTypeFont transcriptFont;
	bool fontsReady = false;
};
