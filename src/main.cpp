#include <assert.h>
#include <algorithm>
#include <complex>
#include <functional>
#include <optional>
#include <random>
#include <string>
#include <stdint.h>
#include <stdio.h>
#include <thread>
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
const int iterations = 7000; //10000 * 2; //5000;
const int orbit_iterations = 5;
const bool AA = true;
const int AA_samples = 10;
const int max_period = 20;

std::mt19937 rng;
std::uniform_real_distribution<fp> ux(-dx/2, dx/2);
std::uniform_real_distribution<fp> uy(-dy/2, dy/2);

std::complex<fp> phi_n(int n, std::complex<fp> z, const std::complex<fp> c) {
	while(n--) {
		z = z * z + c;
	}
	return z;
}

std::complex<fp> phi_prime(const std::complex<fp> z, [[maybe_unused]] const std::complex<fp> c) {
	return 2. * z;
}

std::complex<fp> lambda(const int n, const std::complex<fp> z, const std::complex<fp> c) {
	std::complex<fp> lambda = phi_prime(z, c);
	for(int i = 1; i < n; i++) {
		lambda *= phi_prime(phi_n(i, z, c), c);
	}
	return lambda;
}

bool is_period(const int n, std::complex<fp> z, const std::complex<fp> c) {
	for(int i = 0; i < std::max(n, max_period); i++) {
		if(std::abs(lambda(n, z, c)) >= 1) {
			return false;
		}
		z = z * z + c;
	}
	return true;
}

// returns cycles in orbit or none if the point is outside the set
std::optional<int> mandelbrot(fp x, fp y) {
	/*
	 * Return none for escapees
	 * Return positive integer when period is known
	 * Return zero when period is undetermined
	 */
	std::complex<fp> c = std::complex<fp>(x, y);
	std::complex<fp> z = std::complex<fp>(0, 0);
	int n = iterations;
	while(n-- && std::norm(z) < 4) {
		z = z * z + c;
	}
	if(std::norm(z) > 4) {
		return {};
	}
	/*
	 * Algorithm:
	 * We need to know roots of theta_c^n where n is the exact period of c
	 * Too hard - O(big mess)
	 * Assume we've converged on an attractive fixed point here
	 * Plug it into multiplier equation and check ...?
	 */
	for(int i = 1; i <= max_period; i++) {
		if(is_period(i, z, c)) {
			return i;
		}
	}
	return {};
}

__attribute__((optimize("O1"))) // don't want ffast-math messing with this particular computation
std::tuple<fp, fp> get_coordinates(int i, int j) {
	return {xmin + ((fp)i / w) * (xmax - xmin), ymin + ((fp)j / h) * (ymax - ymin)};
}

pixel_t get_pixel(fp x, fp y) {
	if(let result = mandelbrot(x, y)) {
		let period = result.value();
		assert(period >= 0);
		pixel_t pixel;
		///if(period > 3) {
		///	pixel = {233, 72, 236};
		///} else {
		///	pixel = colors[period];
		///}
		if(period > max_period) {
			pixel = {255, 0, 0};
		} else {
			uint8_t v = 10 * period;
			pixel = {v, v, v};
		}
		return pixel;
	} else {
		return {255, 255, 255};
	}
}

pixel_t sample(fp x, fp y) {
	if(AA) {
		int r = 0, g = 0, b = 0;
		for(int i = 0; i < AA_samples; i++) {
			let color = get_pixel(x + ux(rng), y + uy(rng));
			r += color.r;
			g += color.g;
			b += color.b;
		}
		return {(uint8_t)((fp)r/AA_samples), (uint8_t)((fp)g/AA_samples), (uint8_t)((fp)b/AA_samples)};
	} else {
		return get_pixel(x, y);
	}
}

int main() {
	assert(byte_swap(0x11223344) == 0x44332211);
	assert(byte_swap(pixel_t{0x11, 0x22, 0x33}) == (pixel_t{0x33, 0x22, 0x11}));
	BMP bmp = BMP("test.bmp", w, h);
	//[[maybe_unused]] pixel_t colors[] = {
	//	{0, 0, 0},
	//	{248, 86, 86},
	//	{96, 236, 149},
	//	{72, 144, 236},
	//};
	for(int j = 0; j < h; j++) {
		for(int i = 0; i < w; i++) {
			if(i % 100 == 0) printf("\033[1K\r%0.2f%% %0.2f%%", (fp)(i + j * w) / (w * h) * 100, (fp)i / w * 100);
			if(i % 100 == 0) fflush(stdout);
			let [x, y] = get_coordinates(i, j);
			let color = sample(x, y);
			bmp.set(i, j, color);
		}
	}
	bmp.write();
}

// mariani silver
// can also mirror...
