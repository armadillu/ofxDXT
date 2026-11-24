//
//  ofxDXT_BatchCompressor.cpp
//  ofxImageSequenceVideo_Example
//
//  Created by Oriol Ferrer Mesià on 18/04/2019.
//
//

#include "ofxDXT_BatchCompressor.h"

#if defined( TARGET_OSX ) || defined( TARGET_LINUX )
#include <getopt.h>
#include <dirent.h>
#else
#include <dirent_vs.h>
#endif

#include "ofxDXT.h"

ofxDXT_BatchCompressor::ofxDXT_BatchCompressor(){

}

ofxDXT_BatchCompressor::~ofxDXT_BatchCompressor(){

	//wait for all threads to end
	for(auto & task : tasks) task.wait();

}


string ofxDXT_BatchCompressor::getStatus(){
	string msg = "State: " + string(state == IDLE ? "IDLE" : "COMPRESSING DXT") + "\n";
	msg += ofToString(compressedFiles.size()) + "/" + ofToString(filesToCompress.size() + compressedFiles.size() + tasks.size()) + " [" + ofToString(tasks.size()) + " active tasks]";
	msg += "\n";
	return msg;
}


void ofxDXT_BatchCompressor::compressWholeFolder(const string & path, const string & extension, bool useAlpha, int numThreads){

	if(state == IDLE){

		state = COMPRESSING;
		filesToCompress.clear();
		compressedFiles.clear();
		this->numThreads = std::max(1, numThreads);
		this->useAlpha = useAlpha;
		allSucceeded = true;

		auto fileNames = ofxDXT_BatchCompressor::getImagesAtDirectory(path, extension);
		for(auto & fn : fileNames){
			filesToCompress.push_back(path + "/" + fn);
		}
		std::reverse(filesToCompress.begin(), filesToCompress.end());

		totalFiles = filesToCompress.size();
	}else{
		ofLogError("ofxDXT_BatchCompressor") << "busy! can't compressWholeFolder() now";
	}
}


void ofxDXT_BatchCompressor::update(){

	if(state == COMPRESSING){

		if (totalFiles == compressedFiles.size() ){ //done
			ofLogNotice("ofxDXT_BatchCompressor") << "done compressing images!";
			state = IDLE;
			bool ok = allSucceeded;
			ofNotifyEvent(eventFinished, ok);

		}else{

			//cleanup finshed tasks threads
			for(int i = tasks.size() - 1; i >= 0; i--){
				//see if thread is done, gather results and remove from vector
				std::future_status status = tasks[i].wait_for(std::chrono::microseconds(0));
				if(status == std::future_status::ready){ //thread is done
					try{
						auto results = tasks[i].get();
						allSucceeded = allSucceeded && results.done;
						compressedFiles.push_back(std::move(results.file));
					}catch(const std::exception & e){
						ofLogError("ofxDXT_BatchCompressor") << e.what();
						allSucceeded = false;
						compressedFiles.emplace_back();
					}
					//checked[results.ID] = results; //store check results
					tasks.erase(tasks.begin() + i);
				}
			}

			//spawn new ones
			while(tasks.size() < numThreads && filesToCompress.size()){ //spawn thread
				string file = std::move(filesToCompress.back());
				filesToCompress.pop_back();
				tasks.push_back( std::async(std::launch::async, &ofxDXT_BatchCompressor::compressAsset, this, std::move(file) ));
			}
		}
	}
}


ofxDXT_BatchCompressor::CompressInfo ofxDXT_BatchCompressor::compressAsset(string file){

	ofxDXT_BatchCompressor::CompressInfo inf;
	ofPixels pix;
	if(!ofLoadImage(pix, file)){
		inf.file = std::move(file);
		return inf;
	}
	if(pix.getNumChannels() == 3 && useAlpha) pix.setImageType(OF_IMAGE_COLOR_ALPHA);
	else if(pix.getNumChannels() == 4 && !useAlpha) pix.setImageType(OF_IMAGE_COLOR);
	ofxDXT::Data compressedPix;

	if(pix.getNumChannels() == 3){
		inf.done = ofxDXT::compressRgbPixels(pix, compressedPix);
	}else{
		if(pix.getNumChannels() == 4){
			inf.done = ofxDXT::compressRgbaPixels(pix, compressedPix);
		}else{
			inf.file = std::move(file);
			return inf;
		}
	}
	inf.done = inf.done && ofxDXT::saveToDisk(compressedPix, file + ".dxt");
	inf.file = std::move(file);
	return inf;
}


const char * ofxDXT_BatchCompressor::get_filename_extension(const char *filename) {
	const char *dot = strrchr(filename, '.');
	if(!dot || dot == filename) return "";
	return dot + 1;
}


vector<string> ofxDXT_BatchCompressor::getImagesAtDirectory(const string & path, const string & extension){

	DIR *dir2;
	struct dirent *ent;
	vector<string> fileNames;
	string fullPath = ofToDataPath(path,true);

	const auto imageTypes = {extension};

	if ((dir2 = opendir(fullPath.c_str()) ) != NULL) {

		while ((ent = readdir (dir2)) != NULL) {
			string ext = string(ofxDXT_BatchCompressor::get_filename_extension(ent->d_name));
			bool isVisible = ent->d_name[0] != '.';
			bool isCompatibleType;
			isCompatibleType = std::find(imageTypes.begin(), imageTypes.end(), ext ) != imageTypes.end();
			if ( isVisible && isCompatibleType ){
				fileNames.push_back(string(ent->d_name));
			}
		}
		closedir(dir2);
	}
	std::sort(fileNames.begin(), fileNames.end());
	return fileNames;
}
