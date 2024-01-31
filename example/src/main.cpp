#include "ofMain.h"
#include "ofApp.h"

//========================================================================
int main( ){

	//ofSetCurrentRenderer(ofGLProgrammableRenderer::TYPE);

	ofGLFWWindowSettings winSettings;
	winSettings.numSamples = 8;
	winSettings.setSize(1024, 768);
	winSettings.setGLVersion(3, 2);

	shared_ptr<ofAppBaseWindow> win = ofCreateWindow(winSettings);	// sets up the opengl context!

	ofRunApp(win, shared_ptr<ofBaseApp>(new ofApp()));
	ofRunMainLoop();
}
