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
#include <cstring>

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
	return compressPixelsInternal(rgb8Src, false, dst);
}


bool ofxDXT::compressPixelsInternal(const ofPixels & src, bool isRgba, ofxDXT::Data & dst){

	unsigned char block[64];
	int x, y;

	if(src.getWidth() < 4 || src.getHeight() < 4){
		ofLogError("ofxDXT") << "Can't compressPixelsToData()! src pixels must at least 4 x 4.";
		return false;
	}

	std::size_t w = src.getWidth();
	std::size_t h = src.getHeight();
	if(w > 65532 || h > 65532){
		ofLogError("ofxDXT") << "Image dimensions exceed the DXT file format limit.";
		return false;
	}

	if(w%4 != 0 || h%4 != 0){
		ofLogNotice("ofxDXT") << "Image size not multiple of 4 [" << w << " x " << h << "]! We need to resize!";
		//lets expand to a multiple of 4 size
		int w = ceil(src.getWidth()/4.0f) * 4;
		int h = ceil(src.getHeight()/4.0f) * 4;
		ofLogNotice("ofxDXT") << "New pixels size is [" << w << " x " << h << "]";
	}
	w = (w + 3) & ~std::size_t(3);
	h = (h + 3) & ~std::size_t(3);

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

	const unsigned char * srcData = src.getData();
	unsigned char * dstData = (unsigned char *)dst.getData();

	int isAlpha = isRgba ? 1 : 0;
	for(y = 0; y < h; y += 4){
		for(x = 0; x < w; x += 4){
			if(isRgba && x + 4 <= src.getWidth() && y + 4 <= src.getHeight()){
				for(int row = 0; row < 4; ++row){
					std::memcpy(block + row * 16, srcData + ((y + row) * src.getWidth() + x) * 4, 16);
				}
			}else{
				std::memset(block, 0, sizeof(block));
				const int channels = isRgba ? 4 : 3;
				for(int row = 0; row < 4 && y + row < src.getHeight(); ++row){
					for(int col = 0; col < 4 && x + col < src.getWidth(); ++col){
						std::memcpy(block + (row * 4 + col) * 4, srcData + ((y + row) * src.getWidth() + x + col) * channels, channels);
					}
				}
			}
			stb_compress_dxt_block(dstData, block, isAlpha, STB_DXT_HIGHQUAL);
			dstData += isRgba ? 16 : 8; //advance pointer to next block
		}
	}
	return true;
}


bool ofxDXT::saveToDisk(const ofxDXT::Data & data, const string & path){

	string fullPath = ofToDataPath(path, true);
	DxtHeader head;
	switch (data.getCompressionType()) {
  		case ofxDXT::DXT1: head.type = '1'; break;
  		case ofxDXT::DXT3: head.type = '3'; break;
  		case ofxDXT::DXT5: head.type = '5'; break;
		default: ofLogError() << "invalid DXT type! (DXT" << data.getCompressionType() << ")"; return false;
	}
	if(data.getWidth() <= 0 || data.getHeight() <= 0 || data.getWidth() > 65535 || data.getHeight() > 65535 || data.getWidth() % 4 || data.getHeight() % 4 || data.size() != std::size_t(data.getWidth()) * data.getHeight() / (data.getCompressionType() == DXT1 ? 2 : 1)) return false;
	head.width = data.getWidth();
	head.height = data.getHeight();
	auto myfile = std::fstream(fullPath, std::ios::out | std::ios::binary);

    myfile.write((const char*)&head, sizeof(DxtHeader));
    myfile.write((const char*)data.getData(), data.size());
    myfile.close();
    return bool(myfile);
}


bool ofxDXT::loadFromDisk(const string & path, ofxDXT::Data & data){

	string fullPath = ofToDataPath(path, true);

	//get file size
	struct stat stat_buf;
    int rc = stat(fullPath.c_str(), &stat_buf);
    if(rc != 0 || stat_buf.st_size < 0) return false;
    size_t fileSize = static_cast<size_t>(stat_buf.st_size);

    if(fileSize < sizeof(DxtHeader)){
    		ofLogError() << "Not a valid DXT file! \"" << path << "\"";
    		return false;
	}

	auto myfile = std::fstream(fullPath, std::ios::in | std::ios::binary);
	DxtHeader head;

	myfile.read((char*)&head, sizeof(DxtHeader));
	if(!myfile || std::memcmp(head.compressionType, "DXT", 3) != 0 || head.pipe != '|' || head.width == 0 || head.height == 0 || head.width % 4 || head.height % 4) return false;
	size_t dataSize;
	int dxtType = 0;
	switch (head.type) {
		case '1': dataSize = std::size_t(head.width) * std::size_t(head.height) / std::size_t(2); dxtType = 1; break; //8:1
		case '3': dataSize = std::size_t(head.width) * head.height; dxtType = 3; break;
		case '5': dataSize = std::size_t(head.width) * head.height; dxtType = 5; break;
		default: ofLogError() << "Not a valid DXT file! \"" << path << "\""; return false;
	}

	if(fileSize != sizeof(DxtHeader) + dataSize) return false;
	ofxDXT::Data loaded;
	loaded.allocate(dataSize);
	myfile.read((char*)loaded.getData(), dataSize);
	if(!myfile) return false;
	loaded.setSize(head.width, head.height);
	loaded.setCompressionType(ofxDXT::CompressionType(dxtType));
	data = std::move(loaded);
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
	bool allocated = texture.isAllocated();
	bool sizeMismatch = true;
	bool typeMismatch = true;
	if(allocated){
		auto & texData = texture.getTextureData();
		sizeMismatch = (int)texData.width == 0 || (int)texData.height == 0 || (int)data.getWidth() != (int)texData.width || (int)data.getHeight() != (int)texData.height;
		typeMismatch = texData.glInternalFormat != glCompressionType;
	}
	if(!allocated || sizeMismatch || typeMismatch){
		texture.allocate(data.getWidth(), data.getHeight(), glCompressionType, false /*force GL_TEXTURE_2D*/, type, GL_UNSIGNED_BYTE);
	}
	texture.loadData((unsigned char *)data.getData(), data.getWidth(), data.getHeight(), glCompressionType);
}
