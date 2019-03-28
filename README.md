# ofxDXT

Compress ofPixels into DXT OpenGL textures. Texture compression can be very useful to fit more data into your GPU, but also to be able to stream texture data into your GPU faster.

DXT offers a 4:1 compression ratio.

This addon also allows you to save the compressed data to disk, and load it back. The format in which the data is not standard; usually you would save this data as a .DDS file, but I had to time to look into that, and I didnt really need it either. The file format itself is a 9 byte header followed by the raw data, which is great for realtime loading of textures.

# How to Use

```c++

	//load a test PNG into ofPixels
	ofPixels pix;
	ofLoadImage(pix, "alphaTest.png");

	//create an ofxDXT::Data obj to store compressed texture
	ofxDXT::Data compressedPix;

	//compress RGBA pixels into DXT data
	ofxDXT::compressRgbaPixelsToData(pix, compressedPix);

	ofTexture compressedTexture;
	//load this data into an ofTexture - this sets the right internal GL types
	ofxDXT::loadDataIntoTexture(compressedPix2, compressedTexture);


```
# Required changes in Openframeworks

For ofTexture to play nice with DXT compressed textures, you will need to make some changes to OpenFrameworks's ofTexture class. Specifically, in the ofTexture::allocate() and ofTexture::loadData() methods.  

See these changes [here](https://github.com/local-projects/openFrameworks/commit/974ba1f6eddbbea3c15fd4c30d8024c6e5eabf02).

Basically, you need to use `glCompressedTexImage2D()` instead of `glTexImage2D()` to allocate the compressed DXT texture data, and `glCompressedTexSubImage2D()` instead of `glTexSubImage2D()`. Some other arguments need to be changed as well, so this changes handles that.

These changes will only apply when an ofTexture's `glInternalFormat` is set to `GL_COMPRESSED_RGB_S3TC_DXT*_EXT`, so it should not affect its normal usage.

## License
ofxDXT has been created by Oriol Ferrer Mesià and released under [MIT](http://www.opensource.org/licenses/mit-license.php) license.

ofxDXT uses `stb_dxt.h` from the [stb libs](https://github.com/nothings/stb), which are public domain.


## To Do 

* PR to OpenFramworks to support DXT?
