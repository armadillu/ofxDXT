#include "ofApp.h"
#include <algorithm>
#include <cmath>
#include <thread>

void ofApp::setup(){
	ofBackground(22);
	ofSetFrameRate(60);

	string msg = "\nGL ERROR CODES\n________________________\n";
	msg += "GL_NO_ERROR : " + ofToString(GL_NO_ERROR) + "\n";
	msg += "GL_INVALID_OPERATION : " + ofToString(GL_INVALID_OPERATION) + "\n";
	msg += "GL_INVALID_ENUM : " + ofToString(GL_INVALID_ENUM) + "\n" ;
	msg += "GL_INVALID_VALUE : " + ofToString(GL_INVALID_VALUE) + "\n";
	msg += "GL_STACK_OVERFLOW : " + ofToString(GL_STACK_OVERFLOW) + "\n";
	msg += "GL_STACK_UNDERFLOW : " + ofToString(GL_STACK_UNDERFLOW) + "\n";
	msg += "GL_OUT_OF_MEMORY : " + ofToString(GL_OUT_OF_MEMORY) + "\n";
	ofLogNotice() << msg;

	// Single-image example: compress, save, reload, and upload one PNG.
	ofPixels pix;
	ofLoadImage(pix, "alphaTest.png");
	originalTexture.loadData(pix); //load normal texture from png

	//create data obj to store compressed texture
	ofxDXT::Data compressedPix;

	//compress RGBA pixels into DXT data
	ofxDXT::compressRgbaPixels(pix, compressedPix);

	//save these DXT compressed pixels to disk
	ofxDXT::saveToDisk(compressedPix, "test.dxt");

	//retrieve them back from disk into DXT compressed data
	ofxDXT::Data compressedPix2;
	ofxDXT::loadFromDisk("test.dxt", compressedPix2);

	//load this data into an ofTexture - this sets the right internal GL types
	ofxDXT::loadDataIntoTexture(compressedPix2, singleCompressedTexture);

	//Batch job example
	ofAddListener(compressor.eventFinished, this, &ofApp::onBatchFinished);
	// Compress the preset images in the data/images folder.
	startDirectory(ofToDataPath("images", true));
}

void ofApp::startDirectory(const string & path){
	if(compressor.isBusy() || startPending) return;
	directory = path;
	images.clear();
	extensions = {"png", "gif", "jpg", "jpeg", "PNG", "JPG", "JPEG"};
	nextExtension = 0;
	hadErrors = false;
	status = "Scanning " + directory;
	startPending = true;
}

void ofApp::startNextBatch(){
	startPending = false;
	while(nextExtension < extensions.size()){
		const string & extension = extensions[nextExtension++];
		currentFiles = ofxDXT_BatchCompressor::getImagesAtDirectory(directory, extension);
		if(!currentFiles.empty()){
			const unsigned int cores = std::thread::hardware_concurrency();
			compressor.compressWholeFolder(directory, extension, true, static_cast<int>(std::max(1u, cores)));
			status = "Compressing ." + extension + " images";
			return;
		}
	}
	if(images.empty()) status = hadErrors ? "Compression failed; check the log." : "No images found in data/images.";
	else status = ofToString(images.size()) + " textures loaded" + (hadErrors ? " (some batches failed; check the log)" : "");
}

void ofApp::onBatchFinished(bool & success){
	if(success){
		// Texture uploads must happen on the main thread, after the batch finishes.
		for(const auto & name : currentFiles){
			ofxDXT::Data data;
			if(!ofxDXT::loadFromDisk(directory + "/" + name + ".dxt", data)){
				hadErrors = true;
				continue;
			}
			images.emplace_back();
			images.back().name = name;
			ofxDXT::loadDataIntoTexture(data, images.back().texture);
			if(!images.back().texture.isAllocated()){
				images.pop_back();
				hadErrors = true;
			}
		}
	}else{
		// Do not display old output files from a failed batch.
		hadErrors = true;
	}
	startPending = true;
}

void ofApp::update(){
	compressor.update();
	if(startPending && !compressor.isBusy()) startNextBatch();
}

void ofApp::draw(){

	ofSetBackgroundColor((ofGetFrameNum()/2)%255);

	ofSetColor(255);
	ofDrawBitmapString("Images in data/images are compressed to .dxt files beside the originals.\n" +
		status + (compressor.isBusy() ? "  " + compressor.getStatus() : ""), 20, 24);

	const float margin = 20;
	const float singleTop = 110;
	ofDrawBitmapString("Single image: original / DXT", margin, singleTop);
	if(originalTexture.isAllocated() && singleCompressedTexture.isAllocated()){
		originalTexture.draw(margin, singleTop + 10);
		singleCompressedTexture.draw(margin + originalTexture.getWidth() + 20, singleTop + 10);
	}
	const float top = singleTop + std::max(originalTexture.getHeight(), singleCompressedTexture.getHeight()) + 40;
	ofDrawBitmapString("Batch textures", margin, top - 10);
	float cellWidth = 220, cellHeight = 220;
	for(const auto & image : images){
		cellWidth = std::max(cellWidth, image.texture.getWidth() + 20);
		cellHeight = std::max(cellHeight, image.texture.getHeight() + 35);
	}
	const int columns = std::max(1, static_cast<int>((ofGetWidth() - 2 * margin) / cellWidth));
	for(int i = 0; i < static_cast<int>(images.size()); ++i){
		int row = i / columns;
		float x = margin + (i % columns) * cellWidth;
		float y = top + row * cellHeight;
		const auto & image = images[i];
		ofSetColor(255);
		image.texture.draw(x, y);
		ofDrawBitmapString(image.name, x, y + cellHeight - 12);
	}
}

void ofApp::exit(){
	ofRemoveListener(compressor.eventFinished, this, &ofApp::onBatchFinished);
}
