#pragma once

#include "ofMain.h"
#include "ofxDXT.h"

class ofApp : public ofBaseApp{

public:
	void setup();
	void update();
	void draw();

	ofTexture compressedTex;
	ofTexture tex;
 
};
