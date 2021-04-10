#pragma once
#include <cstring>
#include <iostream>
#include <fstream>
#include <cassert>

class byteArray {
public:
	char* bytes;
	int size;
	inline byteArray()
	{
		size = 0;
		bytes = nullptr;
	}
	inline byteArray(int size, int num)
	{
		this->size = size;
		bytes = new char[this->size]();
		int temp = num;
		for (int i = 0; i < size; i++) {
			bytes[i] = static_cast<char>(temp % 256);
			temp >>= 8;
		}
	}
	inline ~byteArray() {
		delete[] bytes;
		size = 0;
	}
	inline byteArray& operator+=(const byteArray& right)
	{
		int newSize = right.size + size;
		char* newBuf = new char[newSize];
		for (int i = 0; i < size; i++) {
			newBuf[i] = bytes[i];
		}
		for (int i = 0; i < right.size; i++) {
			newBuf[size + i] = right.bytes[i];
		}
		size = newSize;
		delete[] bytes;
		bytes = newBuf;
		return *this;
	}
	inline byteArray(byteArray&& r) noexcept {
		bytes = r.bytes;
		size = r.size;
		r.bytes = nullptr;
		r.size = 0;
	}
	inline byteArray(const byteArray& r) {
		size = r.size;
		bytes = new char[size];
		memcpy(bytes, r.bytes, size);
	}
	inline byteArray& operator=(byteArray&& r) noexcept {
	    if(this == &r){
	        return *this;
	    }
		delete[] bytes;
		bytes = r.bytes;
		size = r.size;
		r.bytes = nullptr;
		r.size = 0;
		return *this;
	}
	inline byteArray& operator=(const byteArray& r) {
		if(this == &const_cast<byteArray&>(r)){
            return *this;
		}
	    size = r.size;
		delete[] bytes;
		bytes = new char[size];
		memcpy(bytes, r.bytes, size);
		return *this;
	}
	inline byteArray operator+(const byteArray& rhs) const{
		byteArray n(*this);
		n += rhs;
		return n;
	}
};

struct Color {
	int red;
	int green;
	int blue;
	explicit inline Color(int r = 0, int g = 0, int b = 0) {
		red = r;
		green = g;
		blue = b;
	}
};

template <int C>
class bitMapImage {
	//file header
	byteArray bfType;
	byteArray bfSize;
	byteArray bfReserved;
	byteArray bfOffBits;
	//image header(windows style)
	byteArray biSize;
	byteArray biWidth;
	byteArray biHeight;
	byteArray biPlanes;
	byteArray biBitCount;
	byteArray biCompression;
	byteArray biSizeImage;
	byteArray biXPelsPerMeter;
	byteArray biYPelsPerMeter;
	byteArray biClrUsed;
	byteArray biClrImportant;
	//color table
	byteArray colorTbl;
	/*byteArray colorTbl2;
	byteArray colorTbl3;*/
	//Pixel data(each 4 bytes for 1 pixel)
	byteArray pixelBuf;
	int w;
	int h;
	int koef;
public:
	inline bitMapImage(int x, int y, Color* colors = nullptr, int size = 0) {
		koef = 0;
	    this->bfType.size = 2;
		w = x;
		h = y;
		delete[] this->bfType.bytes;
		this->bfType.bytes = new char[2];
		this->bfType.bytes[0] = 'B';
		this->bfType.bytes[1] = 'M';
		//this->bfSize = byteArray(4, 66 + x * y * 4);
		this->bfReserved = byteArray(4, 0);
		//this->bfOffBits = byteArray(4, 66);
		this->biSize = byteArray(4, 40);
		this->biWidth = byteArray(4, x);
		this->biHeight = byteArray(4, y);
		this->biPlanes = byteArray(2, 1);
		this->biBitCount = byteArray(2, C);
		this->biCompression = byteArray(4, 0);
		this->biSizeImage = byteArray(4, 0);
		this->biXPelsPerMeter = byteArray(4, 0);
		this->biYPelsPerMeter = byteArray(4, 0);
		this->biClrUsed = byteArray(4, 0);
		this->biClrImportant = byteArray(4, 0);
		//this->colorTbl = byteArray(4, 0b0000011111100000) + byteArray(4, 0b1111100000000000) + byteArray(4, 0b0000000000011111);
		this->koef = ((4 - static_cast<int>(ceil((static_cast<double>(C) / 8) * w)) % 4) % 4) * h;
		this->pixelBuf.size = y * static_cast<int>(ceil(x * (static_cast<double>(C) / 8))) + koef;
		delete[] this->pixelBuf.bytes;
		this->pixelBuf.bytes = new char[this->pixelBuf.size]();
		iLoveColorTable(colors, size);
	}
	void setPixel([[maybe_unused]] int x, [[maybe_unused]] int y, [[maybe_unused]] int colorIndex);
    [[maybe_unused]] void setPixel(int x, int y, Color c);
	void saveToFile([[maybe_unused]] const char* filename);
private:
    [[maybe_unused]] inline byteArray* getBytes() {
		return &pixelBuf;
	}
	inline byteArray fullFile()
	{
		byteArray ans = bfType;
		ans += bfSize;
		ans += bfReserved;
		ans += bfOffBits;
		ans += biSize;
		ans += biWidth;
		ans += biHeight;
		ans += biPlanes;
		ans += biBitCount;
		ans += biCompression;
		ans += biSizeImage;
		ans += biXPelsPerMeter;
		ans += biYPelsPerMeter;
		ans += biClrUsed;
		ans += biClrImportant;
		ans += colorTbl;
		/*ans += colorTbl2;
		ans += colorTbl3;*/
		ans += pixelBuf;
		return ans;
	}
	void iLoveColorTable([[maybe_unused]] [[maybe_unused]] Color* colors = nullptr, [[maybe_unused]] [[maybe_unused]] int size = 0);
};

template<>
[[maybe_unused]] void bitMapImage<32>::setPixel(int x, int y, Color c) {
	if (x >= w || y >= h) {
		throw std::invalid_argument("Can't set this pixel");
	}
	pixelBuf.bytes[y * w * 4 + x * 4 + 2] = static_cast<char>(c.red);
	pixelBuf.bytes[y * w * 4 + x * 4 + 1] = static_cast<char>(c.green);
	pixelBuf.bytes[y * w * 4 + x * 4] = static_cast<char>(c.blue);
	pixelBuf.bytes[y * w * 4 + x * 4 + 3] = 0;
}

template<>
void bitMapImage<32>::saveToFile(const char* filename) {
	std::ofstream out(filename, std::ios::out | std::ios::binary);
	byteArray sourceFile = fullFile();
	out.write(sourceFile.bytes, sourceFile.size);
	out.close();
}

template<>
void bitMapImage<32>::iLoveColorTable([[maybe_unused]] Color* colors, [[maybe_unused]] int size) {
	this->colorTbl = byteArray(4, 0x00FF0000) + byteArray(4, static_cast<int>(0xFF000000)) + byteArray(4, 0x0000FF00);
	this->bfOffBits = byteArray(4, 66);
	this->bfSize = byteArray(4, 66 + static_cast<int>(ceil(this->w * this->h * (static_cast<double>(32) / 8))) + this->koef);
}

template<>
[[maybe_unused]] void bitMapImage<24>::setPixel(int x, int y, Color c) {
	if (x >= w || y >= h) {
		throw std::invalid_argument("Can't set this pixel");
	}
	int koef1 = (koef / h) * y;
	pixelBuf.bytes[y * w * 3 + x * 3 + 2 + koef1] = static_cast<char>(c.red);
	pixelBuf.bytes[y * w * 3 + x * 3 + 1 + koef1] = static_cast<char>(c.green);
	pixelBuf.bytes[y * w * 3 + x * 3 + koef1] = static_cast<char>(c.blue);
}

template<>
void bitMapImage<24>::saveToFile(const char* filename) {
	std::ofstream out(filename, std::ios::out | std::ios::binary);
	byteArray sourceFile = fullFile();
	out.write(sourceFile.bytes, sourceFile.size);
	out.close();
}

template<>
void bitMapImage<24>::iLoveColorTable([[maybe_unused]] Color* colors,[[maybe_unused]] int size) {
	this->colorTbl = byteArray();
	this->bfOffBits = byteArray(4, 54);
	this->bfSize = byteArray(4, 54 + static_cast<int>(ceil(this->w * this->h * (static_cast<double>(24) / 8))) + this->koef);
}

template<>
[[maybe_unused]] void bitMapImage<16>::setPixel(int x, int y, Color c) {
	if (x >= w || y >= h) {
		throw std::invalid_argument("Can't set this pixel");
	}
	int koef1 = (koef / h) * y;
	c.red = std::min(c.red, 31);
	c.green = std::min(c.green, 63);
	c.blue = std::min(c.blue, 31);
	int t1 = c.red << 3;
	int t11 = t1 + (c.green >> 3);
	pixelBuf.bytes[y * w * 2 + x * 2 + 1 + koef1] = static_cast<char>(t11);
	int tfuck = (c.green >> 3) << 3;
	int t2 = (c.green - tfuck) << 5;
	int t21 = t2 + c.blue;
	pixelBuf.bytes[y * w * 2 + x * 2 + koef1] = static_cast<char>(t21);
}

template<>
void bitMapImage<16>::saveToFile(const char* filename) {
	std::ofstream out(filename, std::ios::out | std::ios::binary);
	byteArray sourceFile = fullFile();
	out.write(sourceFile.bytes, sourceFile.size);
	out.close();
}

template<>
void bitMapImage<16>::iLoveColorTable([[maybe_unused]] Color* colors, [[maybe_unused]] int size) {
	this->colorTbl = byteArray(4, 0b0000011111100000) + byteArray(4, 0b1111100000000000) + byteArray(4, 0b0000000000011111);
	this->bfOffBits = byteArray(4, 66);
	this->bfSize = byteArray(4, 66 + static_cast<int>(ceil(this->w * this->h * (static_cast<double>(16) / 8))) + this->koef);
}

template<>
void bitMapImage<8>::setPixel(int x, int y, int colorIndex) {
	if (x >= w || y >= h) {
		throw std::invalid_argument("Can't set this pixel");
	}
	int koef1 = (koef / h) * y;
	pixelBuf.bytes[y * w + x + koef1] = static_cast<char>(colorIndex);
}

template<>
void bitMapImage<8>::saveToFile(const char* filename) {
	std::ofstream out(filename, std::ios::out | std::ios::binary);
	byteArray sourceFile = fullFile();
	out.write(sourceFile.bytes, sourceFile.size);
	out.close();
}

template<>
void bitMapImage<8>::iLoveColorTable(Color* colors, int size) {
	if (colors == nullptr)
		throw std::invalid_argument("Giving nullptr is baaaaad. You shouldn't give nullptr because it's baaaaad, Mkay?");
	if (size < 256)
		throw std::invalid_argument("Giving less than 256 colors is baaaaad. You shouldn't give less than 256 colors because it's baaaaad, Mkay?");
	this->colorTbl = byteArray();
	for (int i = 0; i < 256; i++) {
		this->colorTbl += byteArray(1, colors[i].blue) + byteArray(1, colors[i].green) + byteArray(1, colors[i].red) + byteArray(1, 0);
	}
	this->bfOffBits = byteArray(4,54 + 256 * 4);
	this->bfSize = byteArray(4, 54 + 256 * 4 + static_cast<int>(ceil(this->w * this->h * (static_cast<double>(8) / 8))) + this->koef);
}

template<>
void bitMapImage<4>::setPixel(int x, int y, int colorIndex) {
	if (x >= w || y >= h) {
		throw std::invalid_argument("Can't set this pixel");
	}
	int koef1 = (koef / h) * y;
	unsigned char num = pixelBuf.bytes[y * static_cast<int>(std::ceil(static_cast<double>(w) / 2)) + x / 2 + koef1];
	if (x % 2 == 0) {
		num &= 0b00001111;
		num += (colorIndex & 0b00001111) << 4;
	}
	else {
		num &= 0b11110000;
		num += (colorIndex & 0b00001111);
	}
	pixelBuf.bytes[static_cast<int>(y * std::ceil(static_cast<double>(w) / 2)) + x / 2 + koef1] = static_cast<char>(num);
}

template<>
void bitMapImage<4>::saveToFile(const char* filename) {
	std::ofstream out(filename, std::ios::out | std::ios::binary);
	byteArray sourceFile = fullFile();
	out.write(sourceFile.bytes, sourceFile.size);
	out.close();
}

template<>
void bitMapImage<4>::iLoveColorTable(Color* colors, int size) {
	if (colors == nullptr)
		throw std::invalid_argument("Giving nullptr is baaaaad. You shouldn't give nullptr because it's baaaaad, Mkay?");
	if (size < 16)
		throw std::invalid_argument("Giving less than 16 colors is baaaaad. You shouldn't give less than 16 colors because it's baaaaad, Mkay?");
	this->colorTbl = byteArray();
	for (int i = 0; i < 16; i++) {
		this->colorTbl += byteArray(1, colors[i].blue) + byteArray(1, colors[i].green) + byteArray(1, colors[i].red) + byteArray(1, 0);
	}
	this->bfOffBits = byteArray(4, 54 + 16 * 4);
	this->bfSize = byteArray(4, 54 + 16 * 4 + static_cast<int>(ceil(this->w * this->h * (static_cast<double>(4) / 8))) + this->koef);
}

template<>
void bitMapImage<1>::setPixel(int x, int y, int colorIndex) {
	if (x >= w || y >= h) {
		throw std::invalid_argument("Can't set this pixel");
	}
	int koef1 = (koef / h) * y;
	unsigned char num = pixelBuf.bytes[y * static_cast<int>(std::ceil(static_cast<double>(w) / 8)) + x / 8 + koef1];
	switch (x % 8)
	{
	case 0:
		num &= 0b01111111;
		num += (colorIndex & 0b00000001) << 7;
		break;
	case 1:
		num &= 0b10111111;
		num += (colorIndex & 0b00000001) << 6;
		break;
	case 2:
		num &= 0b11011111;
		num += (colorIndex & 0b00000001) << 5;
		break;
	case 3:
		num &= 0b11101111;
		num += (colorIndex & 0b00000001) << 4;
		break;
	case 4:
		num &= 0b11110111;
		num += (colorIndex & 0b00000001) << 3;
		break;
	case 5:
		num &= 0b11111011;
		num += (colorIndex & 0b00000001) << 2;
		break;
	case 6:
		num &= 0b11111101;
		num += (colorIndex & 0b00000001) << 1;
		break;
	case 7:
		num &= 0b11111110;
		num += (colorIndex & 0b00000001);
		break;
	}
	pixelBuf.bytes[y * static_cast<int>(std::ceil(static_cast<double>(w) / 8)) + x / 8 + koef1] = static_cast<char>(num);
}

template<>
void bitMapImage<1>::saveToFile(const char* filename) {
	std::ofstream out(filename, std::ios::out | std::ios::binary);
	byteArray sourceFile = fullFile();
	out.write(sourceFile.bytes, sourceFile.size);
	out.close();
}

template<>
void bitMapImage<1>::iLoveColorTable(Color* colors, int size) {
	if (colors == nullptr)
		throw std::invalid_argument("Giving nullptr is baaaaad. You shouldn't give nullptr because it's baaaaad, Mkay?");
	if (size < 2)
		throw std::invalid_argument("Giving less than 2 colors is baaaaad. You shouldn't give less than 2 colors because it's baaaaad, Mkay?");
	this->colorTbl = byteArray();
	for (int i = 0; i < 2; i++) {
		this->colorTbl += byteArray(1, colors[i].blue) + byteArray(1, colors[i].green) + byteArray(1, colors[i].red) + byteArray(1, 0);
	}
	this->bfOffBits = byteArray(4, 54 + 2 * 4);
	this->bfSize = byteArray(4, 54 + 2 * 4 + static_cast<int>(ceil(this->w * this->h * (static_cast<double>(1) / 8))) + this->koef);
}

template <int C>
void bitMapImage<C>::setPixel([[maybe_unused]] int x, [[maybe_unused]] int y, [[maybe_unused]] int colorIndex){
    fprintf(stderr, "%d is not a valid bit count\n", C);
    assert(false);
    //static_assert(false, "Giving any other bit value then 1,4,8,16,24,32 is baaaaad. You shouldn't give any other bit value then 1,4,8,16,24,32 because it's baaaaad, Mkay?");
}

template <int C>
void bitMapImage<C>::saveToFile([[maybe_unused]] const char* filename) {
    fprintf(stderr, "%d is not a valid bit count\n", C);
	assert(false);
    //static_assert(false, "Giving any other bit value then 1,4,8,16,24,32 is baaaaad. You shouldn't give any other bit value then 1,4,8,16,24,32 because it's baaaaad, Mkay?");
}

template <int C>
void bitMapImage<C>::iLoveColorTable([[maybe_unused]] Color* colors, [[maybe_unused]] int size) {
    fprintf(stderr, "%d is not a valid bit count\n", C);
    assert(false);
	//assert(false, "Giving any other bit value then 1,4,8,16,24,32 is baaaaad. You shouldn't give any other bit value then 1,4,8,16,24,32 because it's baaaaad, Mkay?");
}