//
//  ofxDXT.cpp
//  BasicSketch
//
//  Created by Oriol Ferrer Mesià on 28/03/2019.
//
//

#include "ofxDXT.h"

#define STB_DXT_IMPLEMENTATION
#include "../libs/stb/stb_dxt.h"
#undef STB_DXT_IMPLEMENTATION

#include <sys/stat.h>

static int imin(int x, int y) { return (x < y) ? x : y; }

static void extractBlock(const unsigned char *src, int x, int y, int w, int h, unsigned char *block){

	int i, j;
	if ((w-x >=4) && (h-y >=4)){

		src += x*4;
		src += y*w*4;
		for (i=0; i < 4; ++i)
		{
			*(unsigned int*)block = *(unsigned int*) src; block += 4; src += 4;
			*(unsigned int*)block = *(unsigned int*) src; block += 4; src += 4;
			*(unsigned int*)block = *(unsigned int*) src; block += 4; src += 4;
			*(unsigned int*)block = *(unsigned int*) src; block += 4;
			src += (w*4) - 12;
		}
		return;
	}

	int bw = imin(w - x, 4);
	int bh = imin(h - y, 4);
	int bx, by;

	const int rem[] =
	{
		0, 0, 0, 0,
		0, 1, 0, 1,
		0, 1, 2, 0,
		0, 1, 2, 3
	};

	for(i = 0; i < 4; ++i){
		by = rem[(bh - 1) * 4 + i] + y;
		for(j = 0; j < 4; ++j){
			bx = rem[(bw - 1) * 4 + j] + x;
			block[(i * 4 * 4) + (j * 4) + 0] = src[(by * (w * 4)) + (bx * 4) + 0];
			block[(i * 4 * 4) + (j * 4) + 1] = src[(by * (w * 4)) + (bx * 4) + 1];
			block[(i * 4 * 4) + (j * 4) + 2] = src[(by * (w * 4)) + (bx * 4) + 2];
			block[(i * 4 * 4) + (j * 4) + 3] = src[(by * (w * 4)) + (bx * 4) + 3];
		}
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool ofxDXT::compressRgbaPixels(const ofPixels & rgba8Src, ofxDXT::Data & dst){

	int channels = rgba8Src.getNumChannels();
	if (channels != 4){
		ofLogError("ofxDXT") << "Can't compressRgbaPixels()! src pixels must be RGBA8.";
		return false;
	}
	return compressPixelsInternal(rgba8Src, true, dst);
}


bool ofxDXT::compressRgbPixels(const ofPixels & rgb8Src, ofxDXT::Data & dst){

	int channels = rgb8Src.getNumChannels();
	if (channels != 3){
		ofLogError("ofxDXT") << "Can't compressRgbPixels()! src pixels must be RGB8.";
		return false;
	}
	ofPixels rgba = rgb8Src;
	rgba.setImageType(OF_IMAGE_COLOR_ALPHA);
	return compressPixelsInternal(rgba, false, dst);
}


bool ofxDXT::compressPixelsInternal(const ofPixels & src, bool isRgba, ofxDXT::Data & dst){

	unsigned char block[64];
	int x, y;

	if(src.getWidth() < 4 || src.getHeight() < 4){
		ofLogError("ofxDXT") << "Can't compressPixelsToData()! src pixels must at least 4 x 4.";
		return false;
	}

	ofPixels pixGrow;
	ofPixels * srcPix = (ofPixels *)&src;

	std::size_t w = srcPix->getWidth();
	std::size_t h = srcPix->getHeight();

	if(w%4 != 0 || h%4 != 0){
		ofLogNotice("ofxDXT") << "Image size not multiple of 4 [" << w << " x " << h << "]! We need to resize!";
		//lets expand to a multiple of 4 size
		int w = ceil(src.getWidth()/4.0f) * 4;
		int h = ceil(src.getHeight()/4.0f) * 4;
		ofLogNotice("ofxDXT") << "New pixels size is [" << w << " x " << h << "]";
		pixGrow.allocate(w, h, 4);
		memset(pixGrow.getData(), 0, pixGrow.getTotalBytes()); //zero out all data RGBA[0000]
		src.pasteInto(pixGrow, 0, 0);
		srcPix = &pixGrow;
	}


	//alloc for the compressed pixels,
	if(isRgba){
		dst.allocate(w * h); //we know this will be 1/4 the size of the png pixels count, DXT5 compresses at 4:1 ratio
	}else{
		//we drop the alpha entirely
		//we know this will be 1/8 the size of the png pixels count, DXT1 compresses at 8:1 ratio
		dst.allocate( std::size_t(w * h) / std::size_t(2) );
	}
	dst.setSize(w, h); //in pixels (real texture pixel size, not data size)
	dst.setCompressionType(isRgba ? ofxDXT::DXT5 : ofxDXT::DXT1);

	const unsigned char * srcData = (const unsigned char *)srcPix->getData();
	unsigned char * dstData = (unsigned char *)dst.getData();

	int isAlpha = isRgba ? 1 : 0;
	for(y = 0; y < h; y += 4){
		for(x = 0; x < w; x += 4){
			extractBlock(srcData, x, y, w, h, block);
			stb_compress_dxt_block(dstData, block, isAlpha, STB_DXT_HIGHQUAL);
			dstData += isRgba ? 16 : 8; //advance pointer to next block
		}
	}
	return true;
}


void ofxDXT::saveToDisk(ofxDXT::Data & data, const string & path){

	string fullPath = ofToDataPath(path, true);
	auto myfile = std::fstream(fullPath, std::ios::out | std::ios::binary);

	DxtHeader head;
	switch (data.getCompressionType()) {
  		case ofxDXT::DXT1: head.type = '1'; break;
  		case ofxDXT::DXT3: head.type = '3'; break;
  		case ofxDXT::DXT5: head.type = '5'; break;
  		default: ofLogError() << "invalid DXT type! (DXT" << data.getCompressionType() << ")"; return;
	}
	head.width = data.getWidth();
	head.height = data.getHeight();

    myfile.write((const char*)&head.compressionType[0], sizeof(DxtHeader));
    myfile.write((const char*)data.getData(), data.size());
    myfile.close();
}


bool ofxDXT::loadFromDisk(const string & path, ofxDXT::Data & data){

	string fullPath = ofToDataPath(path, true);

	//get file size
	struct stat stat_buf;
    size_t rc = stat(fullPath.c_str(), &stat_buf);
    size_t fileSize = rc == 0 ? stat_buf.st_size : -1;

    if(fileSize < sizeof(DxtHeader)){
    		ofLogError() << "Not a valid DXT file! \"" << path << "\"";
    		return false;
	}

	auto myfile = std::fstream(fullPath, std::ios::in | std::ios::binary);
	DxtHeader head;

	myfile.read((char*)&head.compressionType[0] , sizeof(DxtHeader));
	size_t dataSize;
	int dxtType = 0;
	switch (head.type) {
		case '1': dataSize = std::size_t(head.width) * std::size_t(head.height) / std::size_t(2); dxtType = 1; break; //8:1
		case '3': dataSize = head.width * head.height; dxtType = 3; break;
		case '5': dataSize = head.width * head.height; dxtType = 5; break;
		default: ofLogError() << "Not a valid DXT file! \"" << path << "\""; return false;
	}

	data.allocate(dataSize);
	myfile.read((char*)data.getData(), dataSize);
	data.setSize(head.width, head.height);
	data.setCompressionType(ofxDXT::CompressionType(dxtType));
	return true;
}


GLint ofxDXT::getGlTypeForCompression(ofxDXT::CompressionType t){
	switch (t) {
		case ofxDXT::DXT1: return GL_COMPRESSED_RGB_S3TC_DXT1_EXT;
		case ofxDXT::DXT3: return GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
		case ofxDXT::DXT5: return GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
		default:
			ofLogError("ofxDXT") << "unknown compression type! (" << t << ")";
			return GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
	}
}


void ofxDXT::loadDataIntoTexture(const ofxDXT::Data & data, ofTexture & texture){
	GLint glCompressionType = ofxDXT::getGlTypeForCompression(data.getCompressionType());
	GLint type = data.getCompressionType() == ofxDXT::DXT1 ? GL_RGB8 : GL_RGBA8;
	//allocate texture with DXT5 internals
	texture.allocate(data.getWidth(), data.getHeight(), glCompressionType, false /*force GL_TEXTURE_2D*/, type, GL_UNSIGNED_BYTE);
	//load texture from a PNG compressed on-the-fly to DDS (rygCompress)
	texture.loadData((unsigned char *)data.getData(), data.getWidth(), data.getHeight(), glCompressionType);
}
