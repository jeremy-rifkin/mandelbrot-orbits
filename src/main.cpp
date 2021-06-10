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
const fp dx = (xmax - xmin) / w;
const fp dy = (ymax - ymin) / h;
const int iterations = 300; //5000;
const int orbit_iterations = 5;

// 0 for kmeans
// 1 for threshold
// 2 for fixed threshold
#define MODE 2

/*std::string to_string(const std::complex<fp>& c) {
	return std::to_string(c.real()) + " + " + std::to_string(c.imag()) + "i";
}

void trace(std::complex<fp> z, const std::complex<fp> c, int n) {
	for(int i = 0; i < n; i++) {
		z = z * z + c;
		printf("z %02d : %s\n", i, to_string(z).c_str());
	}
}

std::complex<fp> theta_prime(std::complex<fp> z) {
	return 2. * z;
	using namespace std::complex_literals;
	foo(.1 + .1i); // in the mandelbrot set
	foo(.1 + .7i); // not in the mandelbrot set
	return 0;
}

void foo(const std::complex<fp> c) {
	using namespace std::complex_literals;
	std::complex<fp> z;
	z = (1. + std::sqrt(1. - 4. * c)) / 2.;
	printf("start: %s\n", to_string(c).c_str());
	printf("z    : %s\n", to_string(z).c_str());
	trace(z, c, 4);
	printf("t'   : %.2f  (<1?)\n", std::abs(theta_prime(z)));
	printf("--------------\n");
	z = (1. - std::sqrt(1. - 4. * c)) / 2.;
	printf("start: %s\n", to_string(c).c_str());
	printf("z    : %s\n", to_string(z).c_str());
	trace(z, c, 4);
	printf("t'   : %.2f  (<1?)\n", std::abs(theta_prime(z)));
	printf("--------------\n");
}*/

// returns cycles in orbit or none if the point is outside the set
std::optional<int> mandelbrot(fp x, fp y) {
	std::complex<fp> c = std::complex<fp>(x, y);
	{
		std::complex<fp> z;
		z = (1. + std::sqrt(1. - 4. * c)) / 2.;
		if(std::abs(2. * z) < 1) {
			return -1;
		}
		z = (1. - std::sqrt(1. - 4. * c)) / 2.;
		if(std::abs(2. * z) < 1) {
			return -1;
		}
	}
	std::complex<fp> z = std::complex<fp>(0, 0);
	int n = iterations;
	while(n-- && std::norm(z) < 4) {
		z = z * z + c;
	}
	if(std::norm(z) > 4) {
		return {};
	}
	return 10;
	
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

__attribute__((optimize("O1"))) // don't want ffast-math messing with this particular computation
std::tuple<fp, fp> get_coordinates(int i, int j) {
	return {xmin + ((fp)i / w) * (xmax - xmin), ymin + ((fp)j / h) * (ymax - ymin)};
}

int main() {
	assert(byte_swap(0x11223344) == 0x44332211);
	assert(byte_swap(pixel_t{0x11, 0x22, 0x33}) == (pixel_t{0x33, 0x22, 0x11}));
	BMP bmp = BMP("test.bmp", w, h);
	//pixel_t colors[] = {
	//	{}
	//};
	for(int j = 0; j <= h / 2; j++) {
		fp y = ymin + ((fp)j / h) * (ymax - ymin);
		let [_, y2] = get_coordinates(0, j);
		printf("%0.20f %0.20f\n", y, y2);
	}
	//return 1;
	//for(int j = 0; j <= h / 2; j++) {
	for(int j = 0; j < h; j++) {
		for(int i = 0; i < w; i++) {
			if(i % 100 == 0) printf("\033[1K\r%0.2f%% %0.2f%%", (fp)(i + j * w) / (w * h) * 100, (fp)i / w * 100);
			let [x, y] = get_coordinates(i, j);
			if(let result = mandelbrot(x, y)) {
				let period = result.value();
				if(period > 255 / 20) {
					period = 255 / 20;
				}
				uint8_t v = (uint8_t)(255 - period * 20);
				pixel_t pixel = {v, v, v};
				if(period == -1) {
					pixel = {248, 86, 86};
				}
				bmp.set(i, j, pixel);
				//bmp.set(i, h - j - 1, pixel);
			} else {
				bmp.set(i, j, {255, 255, 255});
				//bmp.set(i, h - j - 1, {255, 255, 255});
			}
		}
		//bmp.write(); // debug stuff
	}
	bmp.write();
}

// mariani silver
// can also mirror...
