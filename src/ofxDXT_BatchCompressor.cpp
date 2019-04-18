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

	//wait for all threads to end
	for(int i = tasks.size() - 1; i >= 0; i--){
		std::future_status status = tasks[i].wait_for(std::chrono::microseconds(0));
		while(status != std::future_status::ready){
			ofSleepMillis(16);
			status = tasks[i].wait_for(std::chrono::microseconds(0));
		}
	}

}


string ofxDXT_BatchCompressor::getStatus(){
	string msg = "State: " + string(state == IDLE ? "IDLE" : "COMPRESSING") + "\n";
	msg += ofToString(compressedFiles.size()) + "/" + ofToString(filesToCompress.size() + compressedFiles.size() + tasks.size()) + " [" + ofToString(tasks.size()) + " active tasks]";
	msg += "\n";
	return msg;
}


void ofxDXT_BatchCompressor::compressWholeFolder(const string & path, const string & extension, int numThreads){

	if(state == IDLE){

		state = COMPRESSING;
		filesToCompress.clear();
		compressedFiles.clear();
		this->numThreads = numThreads;

		auto fileNames = ofxDXT_BatchCompressor::getImagesAtDirectory(path, extension);
		for(auto & fn : fileNames){
			filesToCompress.push_back(path + "/" + fn);
		}

	}else{
		ofLogError("ofxDXT_BatchCompressor") << "busy! can't compressWholeFolder() now";
	}
}


void ofxDXT_BatchCompressor::update(){

	if(state == COMPRESSING){

		if (filesToCompress.size() == compressedFiles.size() ){ //done
			ofLogNotice("ofxDXT_BatchCompressor") << "done compressing images!";
			state = IDLE;
			bool ok = true;
			ofNotifyEvent(eventFinished, ok);

		}else{

			//cleanup finshed tasks threads
			for(int i = tasks.size() - 1; i >= 0; i--){
				//see if thread is done, gather results and remove from vector
				std::future_status status = tasks[i].wait_for(std::chrono::microseconds(0));
				if(status == std::future_status::ready){ //thread is done
					auto results = tasks[i].get();
					compressedFiles.push_back(results.file);
					//checked[results.ID] = results; //store check results
					tasks.erase(tasks.begin() + i);
				}
			}

			//spawn new ones
			while(tasks.size() < numThreads && filesToCompress.size()){ //spawn thread
				string file = filesToCompress.front();
				filesToCompress.erase(filesToCompress.begin());
				tasks.push_back( std::async(std::launch::async, &ofxDXT_BatchCompressor::compressAsset, this, file ));
			}
		}
	}
}


ofxDXT_BatchCompressor::CompressInfo ofxDXT_BatchCompressor::compressAsset(string file){

	ofxDXT_BatchCompressor::CompressInfo inf;
	inf.file = file;
	ofPixels pix;
	ofLoadImage(pix, file);
	ofxDXT::Data compressedPix;

	if(pix.getNumChannels() == 3){
		ofxDXT::compressRgbPixels(pix, compressedPix);
	}else{
		if(pix.getNumChannels() == 4){
			ofxDXT::compressRgbaPixels(pix, compressedPix);
		}else{
			inf.done = false;
			return inf;
		}
	}
	ofxDXT::saveToDisk(compressedPix, file + ".dxt");
	inf.done = true;
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
