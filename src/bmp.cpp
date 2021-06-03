#include "bmp.h"

#include <stdio.h>

bool pixel_t::operator==(const pixel_t& other) const {
	return r == other.r && g == other.g && b == other.b;
}

BMP::BMP(const char* path, std::size_t width, std::size_t height) :
	path(path), width(width), height(height), data(new pixel_t[width * height]) {
	for(std::size_t i = 0; i < width * height; i++) {
		data[i] = {0, 100, 0};
	}
}

BMP::~BMP() {
	delete[] data;
}

void BMP::set(int x, int y, pixel_t p) {
	data[x + y * width] = p;
}

void BMP::write() {
	let* file = fopen(path, "wb");
	// setup header
	header_t header;
	header.info_header.size_of_image_data = 24 * width * height; // no rounding needed
	let padding = (width * 3) % 4;
	if(padding > 0) {
		padding = 4 - padding;
	}
	header.file_size = header.info_header.size_of_header + header.info_header.size_of_image_data + padding * height;
	header.info_header.width = width;
	header.info_header.height = height;
	// perform byteswaps
	if(!is_little_endian()) {
		for(int32_t* ptr : {
			&header.file_size, &header.offset, &header.info_header.size_of_header,
			&header.info_header.width, &header.info_header.height // note: some fields left out
		}) *ptr = byte_swap(*ptr);
		for(int16_t* ptr : {
			&header.info_header.color_planes, &header.info_header.bits_per_pixel
		}) *ptr = byte_swap(*ptr);
	}
	// write header
	fwrite(&header, sizeof(header_t), 1, file);
	// write data
	for(std::size_t y = 0; y < height; y++) {
		for(std::size_t x = 0; x < width; x++) {
			pixel_t pixel = byte_swap(data[x + y * width]);
			fwrite(&pixel, sizeof(pixel_t), 1, file);
		}
		// padding
		fwrite(&padding, 1, padding, file); // content doesn't matter, just write something
	}
	// done
	fclose(file);
}
