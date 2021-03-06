//
//  ofxDXT.h
//  BasicSketch
//
//  Created by Oriol Ferrer Mesià on 28/03/2019.
//
//

#pragma once
#include "ofMain.h"



class ofxDXT{

public:
	
	ofxDXT(){};

	enum CompressionType{ DXT1 = 1, DXT3 = 3, DXT5 = 5 };

	class Data : public ofBuffer{
		public:
			void setSize(int w, int h){width = w; height = h;};
			int getWidth() const {return width;};
			int getHeight() const {return height;};

			CompressionType getCompressionType() const {return type;};
			void setCompressionType(CompressionType t){type = t;}

			void clear(){ ofBuffer::clear(); width = height = 0;};

		protected:
			int width = 0;
			int height = 0;
			CompressionType type;
	};

	//if src pixels width & hight are not multiple of 4, they will be expanded
	//with transparent pixels to be

	static bool compressRgbaPixels(const ofPixels & rgba8Src, ofxDXT::Data & dst); //src pixels must be RGBA 8bpp
	static bool compressRgbPixels(const ofPixels & rgb8Src, ofxDXT::Data & dst); //src pixels must be RGB 8bpp

	static void saveToDisk(ofxDXT::Data & data, const string & path);
	static bool loadFromDisk(const string & path, ofxDXT::Data & data);

	static void loadDataIntoTexture(const ofxDXT::Data & data, ofTexture & texture);

protected:

	static bool compressPixelsInternal(const ofPixels & src, bool isRgba, ofxDXT::Data & dst);

	//DXT FILE HEADER - total header size : 4 + 2 + 2 + 1 = 9 bytes
	// "DXT" : 3bytes CONSTANT
	// type  : 1 byte char - with valid values '1' '3' or '5'
	// width : 2bytes [0-65536]
	// height: 2bytes [0-65536]
	// "|"   : 1 byte CONSTANT
	// data  : all the compressed data (data len calculated from width, height & DXT type)

	static GLint getGlTypeForCompression(ofxDXT::CompressionType t);

	struct DxtHeader{
		char compressionType[3] = {'D', 'X', 'T'};
		unsigned char type = '1';
		uint16_t width = 0;
		uint16_t height = 0;
		unsigned char pipe = '|';
	};

	static void extractBlockRGB(const unsigned char *src, int x, int y, int w, int h, unsigned char *block);
	static void extractBlockRGBA(const unsigned char *src, int x, int y, int w, int h, unsigned char *block);

};
