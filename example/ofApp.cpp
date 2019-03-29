#include "ofApp.h"


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

	//load a test PNG
	ofPixels pix;
	ofLoadImage(pix, "alphaTest.png");
	tex.loadData(pix); //load normal texture from png

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
	ofxDXT::loadDataIntoTexture(compressedPix2, compressedTex);
}


void ofApp::update(){
}


void ofApp::draw(){

	compressedTex.draw(0,0);
	tex.draw(compressedTex.getWidth(), 0);
}
