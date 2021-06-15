#ifndef BMP_H
#define BMP_H

#include <cstddef>
#include <cstdint>
#include <tuple>

#include "utils.h"

struct pixel_t {
	uint8_t r, g, b;
	pixel_t() = default;
	pixel_t(uint8_t v) : r(v), g(v), b(v) {}
	pixel_t(uint8_t r, uint8_t g, uint8_t b) : r(r), g(g), b(b) {}
	pixel_t(std::tuple<uint8_t, uint8_t, uint8_t> c) : r(std::get<0>(c)), g(std::get<1>(c)), b(std::get<2>(c)) {}
	bool operator==(const pixel_t&) const;
	bool operator!=(const pixel_t&) const;
}; static_assert(sizeof(pixel_t) == 3);

class BMP {
	std::size_t width;
	std::size_t height;
	/*
	00 2 BM
	02 4 filesize...?
	06 2 --
	08 2 --
	0A 4 data offset
	0E 4 size of header (40 bytes)
	12 4 width
	16 4 height
	1A 2 color planes (whatever that means) (always 1)
	1C 2 bits per pixel / color depth (24 for us)
	1E 4 compression method (no encoding for us, value zero)
	22 4 size of image data (can be zero if no compression)
	26 4 horizontal resolution (can be zero)
	2A 4 vertical resolution (can be zero)
	2E 4 number of colors in palette (can be zero and inferred from bits per pixel)
	32 4 number of "important" colors (whatever that means) (zero since all colors are important)
	36   image data
	*/
	struct header_t {
		char BM[2] = {'B', 'M'};
		int32_t file_size;
		int32_t _reserved_ = 0;
		int32_t offset = sizeof(header_t);
		struct info_header_t {
			int32_t size_of_header = sizeof(info_header_t);
			int32_t width;
			int32_t height;
			int16_t color_planes = 1;
			int16_t bits_per_pixel = 24;
			int32_t compression_method = 0;
			int32_t size_of_image_data = 0;
			int32_t horizontal_resolution = 0;
			int32_t vertical_resolution = 0;
			int32_t number_of_colors = 0;
			int32_t important_colors = 0;
		} info_header __attribute__((packed)); static_assert(sizeof(info_header_t) == 40);
	} __attribute__((packed)); static_assert(sizeof(header_t) == 54);
	// pixel data
	// note: coords 0,0 are in the bottom left corner
	pixel_t *data;
public:
	BMP(std::size_t, std::size_t);
	BMP(const BMP&) = delete;
	BMP(BMP&&) = delete;
	BMP& operator=(const BMP&) = delete;
	BMP& operator=(BMP&&) = delete;
	~BMP();
	void set(int, int, pixel_t);
	pixel_t get(int, int) const;
	void write(const char*);
};

#endif
