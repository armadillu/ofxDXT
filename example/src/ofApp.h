#pragma once

#include "ofMain.h"
#include "ofxDXT.h"
#include "ofxDXT_BatchCompressor.h"

class ofApp : public ofBaseApp{
public:
	void setup();
	void update();
	void draw();
	void exit();

private:
	struct ImageTexture{
		string name;
		ofTexture texture;
	};

	void startDirectory(const string & path);
	void startNextBatch();
	void onBatchFinished(bool & success);

	ofxDXT_BatchCompressor compressor;
	ofTexture originalTexture;
	ofTexture singleCompressedTexture;
	vector<ImageTexture> images;
	vector<string> extensions;
	vector<string> currentFiles;
	string directory;
	string status;
	std::size_t nextExtension = 0;
	bool startPending = false;
	bool hadErrors = false;
};
