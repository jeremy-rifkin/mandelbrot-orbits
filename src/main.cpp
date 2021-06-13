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

constexpr int w = 1920;
constexpr int h = 1080;
constexpr fp xmin = -2.5;
constexpr fp xmax = 1;
constexpr fp ymin = -1;
constexpr fp ymax = 1;
constexpr fp dx = (xmax - xmin) / w;
constexpr fp dy = (ymax - ymin) / h;
constexpr int iterations = 7000; //10000 * 2; //5000;
constexpr int orbit_iterations = 5;
constexpr bool AA = false;
constexpr int AA_samples = 10;
constexpr int max_period = 20;

// rng seed 0 works for this
//const float h_start = 187;
//const float h_stop = 277;
// rng seed 2 is good for this
constexpr float h_start = 200;
constexpr float h_stop = 330;

pixel_t colors[max_period];
__attribute__((constructor)) void init_colors() {
	std::mt19937 rng(2);
	std::uniform_real_distribution<fp> u(h_start, h_stop);
	for(int i = 0; i < max_period; i++) {
		colors[i] = hsl_to_rgb(u(rng), 0.7, 0.5);
	}
}

enum class render_mode { brute_force, mariani };
constexpr render_mode mode = render_mode::brute_force;

constexpr int border_radius = 5;

std::mt19937 rng;
std::uniform_real_distribution<fp> ux(-dx/2, dx/2);
std::uniform_real_distribution<fp> uy(-dy/2, dy/2);

struct point_descriptor {
	//int escape_time;
	//int period;
	//bool did_escape() {
	//	return escape_time >= iterations;
	//}
	std::optional<int> escape_time;
	int period;
	bool operator==(const point_descriptor& other) const {
		return escape_time == other.escape_time && period == other.period;
	}
	bool operator!=(const point_descriptor& other) const {
		return !operator==(other);
	}
};

// memoization
std::optional<point_descriptor> points[w][h];
bool ms_mask[w][h];
bool wtf_mask[w][h];

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
	for(int p = 1; p <= max_period; p++) {
		if(is_period(p, z, c)) {
			return p;
		}
	}
	return 0;
}

__attribute__((optimize("O1"))) // don't want ffast-math messing with this particular computation
std::tuple<fp, fp> get_coordinates(int i, int j) {
	return {xmin + ((fp)i / w) * (xmax - xmin), ymin + ((fp)j / h) * (ymax - ymin)};
}

/*pixel_t get_pixel(fp x, fp y) {
	if(let result = mandelbrot(x, y)) {
		let period = result.value();
		assert(period >= 0);
		assert(period <= max_period);
		if(period == 0) {
			return 0;
		} else {
			return colors[period - 1];
		}
	} else {
		return 255;
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
}*/

pixel_t _get_color(int i, int j) {
	assert(points[i][j].has_value());
	if(points[i][j].has_value()) {
		if(points[i][j].value().escape_time.has_value()) {
			return 255;
		} else {
			let period = points[i][j].value().period;
			assert(period >= 0);
			assert(period <= max_period);
			if(period == 0) {
				return 0;
			} else {
				return colors[period - 1];
			}
			//let color = points[i][j].value().escape_time.has_value() ? 255 : 0;
			//return color;
		}
	} else {
		//return {255, 127, 38};
		return {255, 0, 0};
	}
}

pixel_t get_color(int i, int j) {
	std::vector<pixel_t> values;
	if(ms_mask[i][j]) values.push_back({255, 127, 38});
	if(wtf_mask[i][j]) values.push_back({255, 0, 0});
	values.push_back(_get_color(i, j));
	int r = 0, g = 0, b = 0;
	for(let pixel : values) {
		r += pixel.r;
		g += pixel.g;
		b += pixel.b;
	}
	int s = values.size();
	return {(uint8_t)(r/s), (uint8_t)(g/s), (uint8_t)(b/s)};
}

point_descriptor get_point(int i, int j) {
	if(points[i][j].has_value()) {
		//wtf_mask[i][j] = true;
		return *points[i][j];
	} else {
		//assert(!ms_mask[i][j]);
		//ms_mask[i][j] = true;
		let [x, y] = get_coordinates(i, j);
		let m = mandelbrot(x, y);
		if(m) {
			points[i][j] = {{}, *m};
		} else {
			points[i][j] = {0, 0};
		}
		return *points[i][j];
	}
}

void mariani_silver(int i, int j, int w, int h) {
	assert(w >= 0 && h >= 0);
	//if(w <= 0 || h <= 0) return;
	if(w <= 4 || h <= 4) {
		// an optimization but also handling an edge case where i + w/2 - 1 ==== i and cdiv(w, 2) + 1 ==== w
		for(int x = i; x < i + w; x++) {
			for(int y = j; y < j + h; y++) {
				points[x][y] = get_point(x, y);
			}
		}
		return;
	}
	std::optional<point_descriptor> pd;
	bool all_same = true;
	for(int x = i; x < i + w; x++) {
		ms_mask[x][j] = true;
		ms_mask[x][j + h - 1] = true;
		let d1 = get_point(x, j);
		let d2 = get_point(x, j + h - 1);
		if(!pd.has_value()) pd = d1;
		if(*pd != d1) all_same = false;
		if(*pd != d2) all_same = false;
	}
	for(int y = j; y < j + h; y++) {
		ms_mask[i][y] = true;
		ms_mask[i + w - 1][y] = true;
		let d1 = get_point(i, y);
		let d2 = get_point(i + w - 1, y);
		if(!pd.has_value()) pd = d1;
		if(*pd != d1) all_same = false;
		if(*pd != d2) all_same = false;
	}
	assert(pd.has_value());
	if(w > cdiv(::w, 2)) all_same = false; // fixme: hack
	if(all_same) {
		for(int x = i + 1; x < i + w - 1; x++) {
			for(int y = j + 1; y < j + h - 1; y++) {
				points[x][y] = *pd;
			}
		}
	} else {
		mariani_silver(i,           j,           w / 2,          h / 2         );
		mariani_silver(i + w/2 - 1, j,           cdiv(w, 2) + 1, h / 2         );
		mariani_silver(i,           j + h/2 - 1, w / 2,          cdiv(h, 2) + 1);
		mariani_silver(i + w/2 - 1, j + h/2 - 1, cdiv(w, 2) + 1, cdiv(h, 2) + 1);
	}
}

int main() {
	// Render pipeline:
	//   Mariani-silver figures out the mandelbrot main-body
	//   
	// todo: explore mariani-silver with escape time awareness
	// todo: parallel mariani-silver
	assert(byte_swap(0x11223344) == 0x44332211);
	assert(byte_swap(pixel_t{0x11, 0x22, 0x33}) == (pixel_t{0x33, 0x22, 0x11}));
	BMP bmp = BMP("test.bmp", w, h);
	//for(int j = 0; j < h; j++) {
	//	for(int i = 0; i < w; i++) {
	//		if(i % 100 == 0) printf("\033[1K\r%0.2f%% %0.2f%%", (fp)(i + j * w) / (w * h) * 100, (fp)i / w * 100);
	//		if(i % 100 == 0) fflush(stdout);
	//		let [x, y] = get_coordinates(i, j);
	//		let m = mandelbrot(x, y);
	//		let color = m ? 0 : 255;
	//		//let color = sample(x, y);
	//		bmp.set(i, j, color);
	//	}
	//}
	mariani_silver(0, 0, w, h);
	puts("finished");
	for(int i = 0; i < w; i++) {
		for(int j = 0; j < h; j++) {
			assert(points[i][j].has_value());
		}
	}
	for(int i = 0; i < w; i++) {
		for(int j = 0; j < h; j++) {
			bool center = points[i][j].value().escape_time.has_value();
			bool has_white = false;
			bool has_non_white = false;
			for(int x = std::max(0, i - 1); x <= std::min(w - 1, i + 1); x++) {
				for(int y = std::max(0, j - 1); y <= std::min(h - 1, j + 1); y++) {
					if(x == i && y == j) continue;
					if(points[x][y].value().escape_time.has_value()) {
						has_white = true;
					} else {
						has_non_white = true;
					}
					if(has_white && has_non_white) goto b;
				}
			}
			b:
			if((center && has_non_white) || (!center && has_white))
			for(int x = std::max(0, i - border_radius); x <= std::min(w - 1, i + border_radius); x++) {
				for(int y = std::max(0, j - border_radius); y <= std::min(h - 1, j + border_radius); y++) {
					if((x-i)*(x-i) + (y-j)*(y-j) > border_radius*border_radius) continue;
					wtf_mask[x][y] = true;
				}
			}
		}
	}
	for(int i = 0; i < w; i++) {
		for(int j = 0; j < h; j++) {
			bmp.set(i, j, get_color(i, j));
		}
	}
	bmp.write();
}
