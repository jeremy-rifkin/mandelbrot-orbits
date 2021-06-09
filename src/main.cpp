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

typedef double fp;

const int w = 1920;
const int h = 1080;
const fp xmin = -2.5;
const fp xmax = 1;
const fp ymin = -1;
const fp ymax = 1;
const int iterations = 5000;
const int orbit_iterations = 5;

// 0 for kmeans
// 1 for threshold
// 2 for fixed threshold
#define MODE 2

// returns cycles in orbit or none if the point is outside the set
std::optional<int> mandelbrot(fp x, fp y) {
	using namespace std::complex_literals;
	std::complex<fp> c = std::complex<fp>(x, y);
	std::complex<fp> z = std::complex<fp>(0, 0);
	int n = iterations;
	while(n-- && std::norm(z) < 4) {
		z = z * z + c;
	}
	if(std::norm(z) > 4) {
		return {};
	}
	#if MODE == 0
	std::vector<std::complex<fp>> points;
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
	std::vector<std::complex<fp>> seed_points;
	for(int i = 0; i < 40; i++) {
		z = z * z + c;
		if(std::norm(z) > 4) {
			return {};
		}
		seed_points.push_back(z);
	}
	fp min_distance = INFINITY;
	for(std::size_t i = 0; i < seed_points.size(); i++) {
		for(std::size_t j = i + 1; j < seed_points.size(); j++) {
			let d = std::abs(seed_points[i] - seed_points[j]);
			if(d < min_distance) {
				min_distance = d;
			}
		}
	}
	// iterate until threshold passed
	fp threshold = min_distance;// * 0.75;
	//int X = 40;
	while(true) {
		z = z * z + c;
		if(std::norm(z) > 4) {
			return {};
		}
		for(int i = (int)seed_points.size() - 1, count = 0; i >= 0; i--, count++) {
			if(std::abs(seed_points[i] - z) <= threshold) {
		//for(let const [count, point] : enumerate(reverse_iter(seed_points))) {
		//	if(std::abs(point - z) <= threshold) {
				return count /* + 1 */;
			}
		}
		seed_points.push_back(z);
		//if(X-- == 0) break; // TODO
	}
	assert(false);
	return -1;
	#elif MODE == 2
	{
		// assume all periods are less than 10
		std::complex<fp> c = std::complex<fp>(x, y);
		std::complex<fp> z = std::complex<fp>(0, 0);
		std::vector<std::complex<fp>> seed_points;
		const float threshold = 1e-9;
		int maxx = 10000;
		while(maxx--) {
			z = z * z + c;
			if(std::norm(z) > 4) {
				return {};
			}
			for(int i = (int)seed_points.size() - 1, count = 0; i >= 0; i--, count++) {
				if(std::abs(seed_points[i] - z) <= threshold) {
					return count /* + 1 */;
				}
			}
			seed_points.push_back(z);
		}
		return -1;
	}
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
			if(i % 100 == 0) printf("\033[1K\r%0.2f%% %0.2f%%", (fp)(i + j * w) / (w * h) * 100, (fp)i / w * 100);
			fp x = xmin + ((fp)i / w) * (xmax - xmin);
			fp y = ymin + ((fp)j / h) * (ymax - ymin);
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
