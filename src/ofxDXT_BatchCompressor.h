//
//  ofxDXT_BatchCompressor.h
//  ofxImageSequenceVideo_Example
//
//  Created by Oriol Ferrer Mesià on 18/04/2019.
//
//

#pragma once
#include "ofMain.h"
#include <future>

class ofxDXT_BatchCompressor{

public:


	ofxDXT_BatchCompressor();

	void update();
	string getStatus();

	//expects a folder full of images, will create one dxt compressed frame for each of them
	void compressWholeFolder(const string & path, const string & extension, bool useAlpha, int numThreads);
	bool isBusy(){return state == COMPRESSING;}

	ofEvent<bool> eventFinished;

	//utils
	static vector<string> getImagesAtDirectory(const string & path, const string & extension);
	
protected:

	enum State{
		IDLE,
		COMPRESSING,
	};

	struct CompressInfo{
		string file;
		bool done = false;
	};

	State state = IDLE;
	int numThreads = 1;
	bool useAlpha;

	vector<string> allFiles;
	vector<string> filesToCompress;
	vector<string> compressedFiles;

	vector<std::future<CompressInfo>> tasks;

	CompressInfo compressAsset(string file);
	static const char *get_filename_extension(const char *filename);
};

