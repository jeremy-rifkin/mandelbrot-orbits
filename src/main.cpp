#include <assert.h>
#include <algorithm>
#include <complex>
#include <functional>
#include <optional>
#include <random>
#include <string>
#include <stdint.h>
#include <stdio.h>
#include <unordered_set>
#include <vector>

#include "bmp.h"
#include "kmeans.h"

const int w = 1920;
const int h = 1080;
const double xmin = -2.5;
const double xmax = 1;
const double ymin = -1;
const double ymax = 1;
const int iterations = 60;
const int orbit_iterations = 5;

// 0 for kmeans
// 1 for threshold
#define MODE 1

// returns cycles in orbit or none if the point is outside the set
std::optional<int> mandelbrot(float x, float y) {
	using namespace std::complex_literals;
	std::complex<float> c = std::complex<float>(x, y);
	std::complex<float> z = std::complex<float>(0, 0);
	int n = iterations;
	while(n-- && std::norm(z) < 4) {
		z = z * z + c;
	}
	if(std::norm(z) > 4) {
		return {};
	}
	#if MODE == 0
	std::vector<std::complex<float>> points;
	points.reserve(orbit_iterations);
	int n = orbit_iterations;
	while(n-- && std::norm(z) < 4) {
		z = z * z + c;
		points.push_back(z);
	}
	let clusters = silhouette_find_clusters(points, 1, 10, 1/*10*/);
	return clusters.size();
	#elif MODE == 1
	// assume all periods are less than 10
	std::vector<std::complex<float>> seed_points;
	for(int i = 0; i < 20; i++) {
		z = z * z + c;
		if(std::norm(z) > 4) {
			return {};
		}
		seed_points.push_back(z);
	}
	float min_distance = INFINITY;
	for(std::size_t i = 0; i < seed_points.size(); i++) {
		for(std::size_t j = i + 1; j < seed_points.size(); j++) {
			if(std::abs(seed_points[i] - seed_points[j]) < min_distance) {
				min_distance = std::abs(seed_points[i] - seed_points[j]);
			}
		}
	}
	// iterate until threshold passed
	float threshold = min_distance;
	//int X = 40;
	while(true) {
		z = z * z + c;
		if(std::norm(z) > 4) {
			return {};
		}
		for(let const [i, point] : enumerate(reverse_iter(seed_points))) {
			if(std::abs(point - z) <= threshold) {
				return i;
			}
		}
		seed_points.push_back(z);
		//if(X-- == 0) break; // TODO
	}
	assert(false);
	return -1;
	#endif
}

int main() {
	assert(byte_swap(0x11223344) == 0x44332211);
	assert(byte_swap(pixel_t{0x11, 0x22, 0x33}) == (pixel_t{0x33, 0x22, 0x11}));
	BMP bmp = BMP("test.bmp", w, h);
	//pixel_t colors[] = {
	//	{}
	//};
	for(int j = 0; j <= h / 2; j++) {
		for(int i = 0; i < w; i++) {
			if(i % 100 == 0) printf("\033[1K\r%0.2f%% %0.2f%%", (float)(i + j * w) / (w * h) * 100, (float)i / w * 100);
			float x = xmin + ((float)i / w) * (xmax - xmin);
			float y = ymin + ((float)j / h) * (ymax - ymin);
			if(let result = mandelbrot(x, y)) {
				let period = result.value();
				if(period > 255 / 20) {
					period = 255 / 20;
				}
				uint8_t v = (uint8_t)(255 - period * 20);
				pixel_t pixel = {v, v, v};
				if(period == -1) {
					pixel = {255, 0, 0};
				}
				bmp.set(i, j, pixel);
				bmp.set(i, h - j - 1, pixel);
			} else {
				bmp.set(i, j, {255, 255, 255});
				bmp.set(i, h - j - 1, {255, 255, 255});
			}
		}
		//bmp.write(); // debug stuff
	}
	bmp.write();
}

// mariani silver
// can also mirror...
