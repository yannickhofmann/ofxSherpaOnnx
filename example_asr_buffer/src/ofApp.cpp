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
#include <chrono>
#include <cmath>
#include <cstdlib>

namespace {

// Read the optional language selected by the install scripts or shell.
std::string getSelectedLanguage() {
	const char * value = std::getenv("OFX_SHERPAONNX_LANG");
	return value == nullptr ? std::string() : std::string(value);
}

bool isCompleteAsrModel(const std::string & encoderPath,
	const std::string & decoderPath,
	const std::string & joinerPath,
	const std::string & tokensPath) {
	return ofFile::doesFileExist(ofToDataPath(encoderPath, true)) &&
		ofFile::doesFileExist(ofToDataPath(decoderPath, true)) &&
		ofFile::doesFileExist(ofToDataPath(joinerPath, true)) &&
		ofFile::doesFileExist(ofToDataPath(tokensPath, true));
}

std::string trimTranscript(std::string text) {
	// Event text may contain line breaks or repeated spaces that are not useful onscreen.
	ofStringReplace(text, "\r", " ");
	ofStringReplace(text, "\n", " ");
	while (text.find("  ") != std::string::npos) {
		ofStringReplace(text, "  ", " ");
	}
	return ofTrim(text);
}

}

//--------------------------------------------------------------
void ofApp::setup() {
	ofSetFrameRate(60);
	ofSetVerticalSync(true);
	ofBackground(5);
	ofSetWindowTitle("ofxSherpaOnnx Voice Toggle");

	fontsReady = titleFont.load("fonts/verdana.ttf", 18, true, true, true) &&
		bodyFont.load("fonts/verdana.ttf", 13, true, true, true) &&
		transcriptFont.load("fonts/verdana.ttf", 30, true, true, true);
	if (!fontsReady) {
		ofLogWarning("ofApp") << "Could not load fonts/verdana.ttf; falling back to bitmap text.";
	}

	// Model and microphone setup are independent and report their own errors.
	setupModel();
	setupAudioInput();
}

//--------------------------------------------------------------
void ofApp::setupModel() {
	// Select German by default. Set OFX_SHERPAONNX_LANG=en to prefer English.
	const std::string requestedLang = getSelectedLanguage();
	const bool preferEnglish = requestedLang == "en";

	const std::string germanFolder = "models/sherpa-onnx-streaming-zipformer-de-kroko-2025-08-06/";
	const std::string englishFolder = "models/online-zipformer-bilingual-zh-en-2023-02-20/";

	std::string modelFolder = preferEnglish ? englishFolder : germanFolder;
	std::string encoderPath = preferEnglish ? modelFolder + "encoder-epoch-99-avg-1.int8.onnx" : modelFolder + "encoder.onnx";
	std::string decoderPath = preferEnglish ? modelFolder + "decoder-epoch-99-avg-1.int8.onnx" : modelFolder + "decoder.onnx";
	std::string joinerPath = preferEnglish ? modelFolder + "joiner-epoch-99-avg-1.int8.onnx" : modelFolder + "joiner.onnx";
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
			tokensPath = modelFolder + "tokens.txt";
			modelType = "zipformer2";
		} else if (!preferEnglish && englishAvailable) {
			ofLogNotice("ofApp") << "German model not found, falling back to English model.";
			modelFolder = englishFolder;
			encoderPath = modelFolder + "encoder-epoch-99-avg-1.int8.onnx";
			decoderPath = modelFolder + "decoder-epoch-99-avg-1.int8.onnx";
			joinerPath = modelFolder + "joiner-epoch-99-avg-1.int8.onnx";
			tokensPath = modelFolder + "tokens.txt";
			modelType = "transducer";
		} else {
			ofLogError("ofApp") << "No complete ASR model found.";
			ofSystemAlertDialog("No complete ASR model found. Run install_all.py first.");
			ofExit();
			return;
		}
	}

	// Convert data-relative paths to absolute paths for the Sherpa-ONNX C API.
	encoderPath = ofToDataPath(encoderPath, true);
	decoderPath = ofToDataPath(decoderPath, true);
	joinerPath = ofToDataPath(joinerPath, true);
	tokensPath = ofToDataPath(tokensPath, true);

	modelReady = sherpaOnnx.setupASR(encoderPath, decoderPath, joinerPath, tokensPath, modelSampleRate, modelType);
	if (!modelReady) {
		ofLogError("ofApp") << "ofxSherpaOnnx setup failed.";
		ofSystemAlertDialog("ofxSherpaOnnx setup failed. Check model files.");
		ofExit();
		return;
	}

	// Recognition events are delivered while the background worker decodes audio.
	ofAddListener(sherpaOnnx.onPartialResult, this, &ofApp::onPartialResultReceived);
	ofAddListener(sherpaOnnx.onFinalResult, this, &ofApp::onFinalResultReceived);

	std::lock_guard<std::mutex> lock(resultMutex);
	statusText = "Ready. Press SPACE, speak, press SPACE.";
}

//--------------------------------------------------------------
void ofApp::setupAudioInput() {
	soundStream.printDeviceList();

	// Try native backends first, then let openFrameworks choose as a fallback.
	std::vector<ofSoundDevice::Api> apis = {
		ofSoundDevice::Api::PULSE,
		ofSoundDevice::Api::ALSA,
		ofSoundDevice::Api::UNSPECIFIED
	};
#ifdef TARGET_WIN32
	apis = {ofSoundDevice::Api::MS_WASAPI, ofSoundDevice::Api::UNSPECIFIED};
#endif

	for (auto api : apis) {
		auto devices = soundStream.getDeviceList(api);
		if (devices.empty()) {
			continue;
		}

		// Prefer the default microphone, or use the first device with an input.
		auto inputDevice = std::find_if(devices.begin(), devices.end(), [](const ofSoundDevice & device) {
			return device.inputChannels > 0 && device.isDefaultInput;
		});

		if (inputDevice == devices.end()) {
			inputDevice = std::find_if(devices.begin(), devices.end(), [](const ofSoundDevice & device) {
				return device.inputChannels > 0;
			});
		}

		if (inputDevice == devices.end()) {
			continue;
		}

		// Record at most stereo; makeMono16kSamples() combines channels later.
		inputChannels = std::max(1u, std::min(2u, inputDevice->inputChannels));
		inputSampleRate = 48000;

		ofSoundStreamSettings settings;
		settings.setInDevice(*inputDevice);
		settings.setInListener(this);
		settings.numInputChannels = inputChannels;
		settings.numOutputChannels = 0;
		settings.sampleRate = inputSampleRate;
		settings.bufferSize = 512;
		settings.numBuffers = 2;

		if (soundStream.setup(settings)) {
			audioReady = true;
			ofLogNotice("ofApp") << "Using input device: " << inputDevice->name
				<< " (" << inputChannels << " channels, " << inputSampleRate << " Hz)";
			break;
		}

		ofLogWarning("ofApp") << "Failed to open input device: " << inputDevice->name;
	}

	if (!audioReady) {
		ofLogWarning("ofApp") << "No input audio device found.";
		std::lock_guard<std::mutex> lock(resultMutex);
		statusText = "No input audio device found.";
	}
}

//--------------------------------------------------------------
void ofApp::update() {
	// Waiting briefly after SPACE avoids cutting off the final sound of a word.
	if (stopPending && isRecording && ofGetElapsedTimef() - stopRequestedAt >= stopGraceSeconds) {
		stopPending = false;
		stopRecordingAndTranscribe();
	}
}

//--------------------------------------------------------------
void ofApp::draw() {
	ofBackgroundGradient(ofColor(18, 12, 20), ofColor(0, 0, 0), OF_GRADIENT_CIRCULAR);

	std::string partial;
	std::string finalText;
	std::string status;
	float seconds = 0.0f;
	float transcribeMs = 0.0f;
	{
		// Copy shared values quickly, then draw without holding the mutex.
		std::lock_guard<std::mutex> lock(resultMutex);
		partial = currentRecognition;
		finalText = finalRecognition;
		status = statusText;
		seconds = lastRecordingSeconds;
		transcribeMs = lastTranscribeMillis;
	}

	if (isRecording) {
		std::lock_guard<std::mutex> lock(audioMutex);
		const size_t frames = inputChannels > 0 ? recordedSamples.size() / inputChannels : recordedSamples.size();
		seconds = inputSampleRate > 0 ? static_cast<float>(frames) / static_cast<float>(inputSampleRate) : 0.0f;
	}

	ofPushStyle();
	const float margin = 86.0f;
	if (fontsReady) {
		drawText(titleFont, "ofxSherpaOnnx voice toggle", margin, 52, ofColor(255, 235, 190, 180));
		drawText(bodyFont, "space toggles recording", margin, ofGetHeight() - 28, ofColor(170, 210, 255, 150));
		drawText(bodyFont, "Status: " + status, margin, 92, ofColor(255, 255, 255, 190));
		drawText(bodyFont, "State: " + stateLabel(), margin, 118, ofColor(255, 255, 255, 190));
		drawText(bodyFont, "Recording: " + ofToString(seconds, 2) + " s", margin, 144, ofColor(255, 255, 255, 190));
		if (transcribeMs > 0.0f) {
			drawText(bodyFont, "Last transcription: " + ofToString(transcribeMs, 1) + " ms", margin, 170, ofColor(255, 255, 255, 190));
		}
	} else {
		ofSetColor(255, 235, 190, 180);
		ofDrawBitmapString("ofxSherpaOnnx voice toggle", margin, 52);
		ofSetColor(170, 210, 255, 150);
		ofDrawBitmapString("space toggles recording", margin, ofGetHeight() - 28);
		ofSetColor(255, 255, 255, 190);
		ofDrawBitmapString("Status: " + status, margin, 92);
		ofDrawBitmapString("State: " + stateLabel(), margin, 118);
		ofDrawBitmapString("Recording: " + ofToString(seconds, 2) + " s", margin, 144);
	}

	const std::string text = finalText.empty() ? partial : finalText;
	const std::string shown = text.empty() ? "press space, speak, press space" : text;
	ofRectangle box(margin, ofGetHeight() * 0.38f, ofGetWidth() - margin * 2.0f, ofGetHeight() * 0.42f);
	if (fontsReady) {
		drawWrappedText(transcriptFont, shown, box.x, box.y, box.width, 42.0f, finalText.empty() ? ofColor(255, 255, 255, 130) : ofColor(255, 236, 198, 235));
	} else {
		ofDrawBitmapStringHighlight(shown, box.x, box.y, ofColor(0, 0, 0, 90), ofColor(255, 236, 198));
	}

	if (isRecording) {
		const float now = ofGetElapsedTimef();
		ofSetColor(255, 80, 100, 90);
		ofNoFill();
		for (int i = 0; i < 5; ++i) {
			const float r = 80.0f + i * 46.0f + std::fmod(now * 36.0f, 46.0f);
			ofDrawCircle(ofGetWidth() * 0.5f, ofGetHeight() * 0.5f, r);
		}
		ofFill();
		ofSetColor(255, 55, 72, 220);
		ofDrawCircle(ofGetWidth() - 42, 42, 10.0f + 4.0f * std::sin(now * 7.0f));
	}
	ofPopStyle();
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
void ofApp::audioIn(ofSoundBuffer & input) {
	// This function runs on the audio thread. Only append samples while recording;
	// transcription happens later on a separate worker thread.
	if (!isRecording || isTranscribing) {
		return;
	}

	const auto & buffer = input.getBuffer();
	std::lock_guard<std::mutex> lock(audioMutex);
	recordedSamples.insert(recordedSamples.end(), buffer.begin(), buffer.end());
}

//--------------------------------------------------------------
void ofApp::startRecording() {
	if (!modelReady || !audioReady || isTranscribing) {
		return;
	}

	// A finished std::thread must be joined before the object can be reused.
	if (transcriptionThread.joinable()) {
		transcriptionThread.join();
	}

	// Every button-triggered recording starts with empty decoder and UI state.
	sherpaOnnx.resetASRStream();
	{
		std::lock_guard<std::mutex> lock(audioMutex);
		recordedSamples.clear();
	}
	{
		std::lock_guard<std::mutex> lock(resultMutex);
		currentRecognition.clear();
		finalRecognition.clear();
		lastRecordingSeconds = 0.0f;
		lastTranscribeMillis = 0.0f;
		statusText = "Listening";
	}

	stopPending = false;
	stopRequestedAt = 0.0f;
	isRecording = true;
	ofLogNotice("ofApp") << "Recording started.";
}

//--------------------------------------------------------------
void ofApp::stopRecordingAndTranscribe() {
	stopPending = false;
	isRecording = false;

	std::vector<float> captured;
	{
		// swap() transfers the complete recording without copying every sample.
		std::lock_guard<std::mutex> lock(audioMutex);
		captured.swap(recordedSamples);
	}

	const unsigned int channels = inputChannels;
	const unsigned int rate = inputSampleRate;
	const size_t frames = channels > 0 ? captured.size() / channels : captured.size();
	{
		std::lock_guard<std::mutex> lock(resultMutex);
		lastRecordingSeconds = rate > 0 ? static_cast<float>(frames) / static_cast<float>(rate) : 0.0f;
		statusText = "Transcribing";
	}

	// Decoding can take noticeable time, so keep it off the drawing thread.
	isTranscribing = true;
	transcriptionThread = std::thread(&ofApp::transcribeRecordedAudio, this, std::move(captured), channels, rate);
	ofLogNotice("ofApp") << "Recording stopped. Transcription started.";
}

//--------------------------------------------------------------
std::vector<float> ofApp::makeMono16kSamples(const std::vector<float> & samples, unsigned int channels, unsigned int sourceSampleRate) {
	if (samples.empty() || channels == 0 || sourceSampleRate == 0) {
		return {};
	}

	ofSoundBuffer recordedBuffer;
	recordedBuffer.setSampleRate(sourceSampleRate);
	recordedBuffer.setNumChannels(channels);
	recordedBuffer.getBuffer() = samples;

	ofSoundBuffer monoBuffer;
	monoBuffer.setSampleRate(sourceSampleRate);
	monoBuffer.setNumChannels(1);
	monoBuffer.resize(recordedBuffer.getNumFrames());

	// Average all channels so the recognizer receives one mono sample per frame.
	for (size_t i = 0; i < recordedBuffer.getNumFrames(); ++i) {
		float sum = 0.0f;
		for (unsigned int channel = 0; channel < channels; ++channel) {
			sum += recordedBuffer.getBuffer()[i * channels + channel];
		}
		monoBuffer.getBuffer()[i] = sum / static_cast<float>(channels);
	}

	if (sourceSampleRate == modelSampleRate) {
		return monoBuffer.getBuffer();
	}

	// Convert the device rate to the 16 kHz rate used to train the model.
	const float speed = static_cast<float>(sourceSampleRate) / static_cast<float>(modelSampleRate);
	const size_t targetFrames = static_cast<size_t>(std::ceil(monoBuffer.getNumFrames() / speed));
	monoBuffer.resampleTo(resampledBuffer, 0, targetFrames, speed);
	resampledBuffer.setSampleRate(modelSampleRate);
	resampledBuffer.setNumChannels(1);
	return resampledBuffer.getBuffer();
}

//--------------------------------------------------------------
void ofApp::transcribeRecordedAudio(std::vector<float> samples, unsigned int channels, unsigned int sourceSampleRate) {
	const auto started = std::chrono::steady_clock::now();
	std::vector<float> mono16k = makeMono16kSamples(samples, channels, sourceSampleRate);

	// Ignore recordings shorter than 200 ms; they rarely contain useful speech.
	if (mono16k.size() < modelSampleRate / 5) {
		std::lock_guard<std::mutex> lock(resultMutex);
		statusText = "Recording too short";
		currentRecognition.clear();
		isTranscribing = false;
		return;
	}

	sherpaOnnx.resetASRStream();
	if (!shouldExit) {
		// Feed 250 ms chunks to mimic the blocks of a live audio stream.
		const size_t chunkSize = modelSampleRate / 4;
		std::vector<float> chunk;
		for (size_t offset = 0; offset < mono16k.size() && !shouldExit; offset += chunkSize) {
			const size_t count = std::min(chunkSize, mono16k.size() - offset);
			chunk.assign(mono16k.begin() + offset, mono16k.begin() + offset + count);
			sherpaOnnx.acceptASRWaveform(chunk);
		}

		// Online models need a little right-context to complete the final word.
		// Synthetic silence is decoded immediately and adds no recording time.
		const size_t tailSamples = static_cast<size_t>(decoderTailSeconds * static_cast<float>(modelSampleRate));
		for (size_t offset = 0; offset < tailSamples && !shouldExit; offset += chunkSize) {
			const size_t count = std::min(chunkSize, tailSamples - offset);
			chunk.assign(count, 0.0f);
			sherpaOnnx.acceptASRWaveform(chunk);
		}
	}

	// Flushing tells the model that no additional audio will arrive.
	std::string text = trimTranscript(sherpaOnnx.finishASRAndGetText());
	if (text.empty()) {
		text = trimTranscript(sherpaOnnx.getCurrentText());
	}

	const auto finished = std::chrono::steady_clock::now();
	const float elapsedMs = std::chrono::duration<float, std::milli>(finished - started).count();

	{
		std::lock_guard<std::mutex> lock(resultMutex);
		finalRecognition = text;
		currentRecognition.clear();
		lastTranscribeMillis = elapsedMs;
		statusText = text.empty() ? "No speech recognized" : "Ready";
	}

	isTranscribing = false;
	ofLogNotice("ofApp") << "Transcription finished in " << elapsedMs << " ms: " << text;
}

//--------------------------------------------------------------
std::string ofApp::stateLabel() const {
	if (isRecording) return "listening";
	if (isTranscribing) return "transcribing";
	if (!modelReady) return "missing model";
	if (!audioReady) return "no microphone";
	return "ready";
}

//--------------------------------------------------------------
void ofApp::onPartialResultReceived(std::string & result) {
	std::lock_guard<std::mutex> lock(resultMutex);
	currentRecognition = trimTranscript(result);
}

//--------------------------------------------------------------
void ofApp::onFinalResultReceived(std::string & result) {
	std::lock_guard<std::mutex> lock(resultMutex);
	finalRecognition = trimTranscript(result);
	currentRecognition.clear();
}

//--------------------------------------------------------------
void ofApp::exit() {
	// Signal the worker first, then wait for it before listeners and objects disappear.
	shouldExit = true;
	isRecording = false;

	soundStream.stop();
	soundStream.close();

	if (transcriptionThread.joinable()) {
		transcriptionThread.join();
	}

	ofRemoveListener(sherpaOnnx.onPartialResult, this, &ofApp::onPartialResultReceived);
	ofRemoveListener(sherpaOnnx.onFinalResult, this, &ofApp::onFinalResultReceived);
}

//--------------------------------------------------------------
void ofApp::keyPressed(int key) {
	if (key == OF_KEY_ESC) {
		ofExit();
	}
}

//--------------------------------------------------------------
void ofApp::keyReleased(int key) {
	if (key != ' ') {
		return;
	}

	if (isTranscribing) {
		ofLogNotice("ofApp") << "Still transcribing. Please wait.";
		return;
	}

	if (isRecording) {
		if (!stopPending) {
			// update() performs the actual stop after the short grace period.
			stopPending = true;
			stopRequestedAt = ofGetElapsedTimef();
			std::lock_guard<std::mutex> lock(resultMutex);
			statusText = "Finishing recording";
		}
	} else {
		startRecording();
	}
}

//--------------------------------------------------------------
void ofApp::mouseMoved(int x, int y) {
}

//--------------------------------------------------------------
void ofApp::mouseDragged(int x, int y, int button) {
}

//--------------------------------------------------------------
void ofApp::mousePressed(int x, int y, int button) {
}

//--------------------------------------------------------------
void ofApp::mouseReleased(int x, int y, int button) {
}

//--------------------------------------------------------------
void ofApp::mouseEntered(int x, int y) {
}

//--------------------------------------------------------------
void ofApp::mouseExited(int x, int y) {
}

//--------------------------------------------------------------
void ofApp::windowResized(int w, int h) {
}

//--------------------------------------------------------------
void ofApp::dragEvent(ofDragInfo dragInfo) {
}

//--------------------------------------------------------------
void ofApp::gotMessage(ofMessage msg) {
}
