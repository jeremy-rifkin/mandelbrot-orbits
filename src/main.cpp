#include <assert.h>
#include <algorithm>
#include <atomic>
#include <chrono>
#include <complex>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <optional>
#include <queue>
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

constexpr int w = 1920; // * 4;
constexpr int h = 1080; // * 4;
constexpr fp xmin = -2.5;
constexpr fp xmax = 1;
constexpr fp ymin = -1;
constexpr fp ymax = 1;
constexpr fp dx = (xmax - xmin) / w;
constexpr fp dy = (ymax - ymin) / h;

constexpr int iterations = 7000; //10000 * 2; //5000;
constexpr int max_period = 20;

constexpr bool AA = true;
constexpr int AA_samples = 10;
constexpr int border_radius = 5;
thread_local std::mt19937 rng;
std::uniform_real_distribution<fp> ux(-dx/2, dx/2);
std::uniform_real_distribution<fp> uy(-dy/2, dy/2);

enum class render_mode { brute_force, mariani };
constexpr render_mode mode = render_mode::mariani;

constexpr bool debug_info = true;

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
	for(let& color : colors) {
		color = hsl_to_rgb(u(rng), 0.7, 0.5);
	}
}

struct point_descriptor {
	bool escaped;
	int escape_time;
	int period;
	point_descriptor(bool escaped, int escape_time, int period) : escaped(escaped), escape_time(escape_time), period(period) {}
	bool operator==(const point_descriptor& other) const {
		if(escaped) {
			return other.escaped && escape_time == other.escape_time;
		} else {
			return period == other.period;
		}
		//return escaped == other.escaped && escape_time == other.escape_time && period == other.period;
		//return (escape_time && other.escape_time) || period == other.period;
		//return period == other.period;
	}
	bool operator!=(const point_descriptor& other) const {
		return !operator==(other);
	}
};

// memoization
atomic_optional<point_descriptor> points[w][h];
bool ms_mask[w][h];
bool aa_mask[w][h];

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
point_descriptor mandelbrot(fp x, fp y) {
	/*
	 * Return none for escapees
	 * Return positive integer when period is known
	 * Return zero when period is undetermined
	 */
	std::complex<fp> c = std::complex<fp>(x, y);
	std::complex<fp> z = std::complex<fp>(0, 0);
	int i = 0;
	while(i < iterations && std::norm(z) < 4) {
		z = z * z + c;
		i++;
	}
	if(std::norm(z) > 4) {
		return {true, i, -1};
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
			return {false, 0, p};
		}
	}
	return {false, 0, 0};
}

__attribute__((optimize("O1"))) // don't want ffast-math messing with this particular computation
std::tuple<fp, fp> get_coordinates(int i, int j) {
	return {xmin + ((fp)i / w) * (xmax - xmin), ymin + ((fp)j / h) * (ymax - ymin)};
}

pixel_t get_pixel(fp x, fp y) {
	if(let result = mandelbrot(x, y); !result.escaped) {
		let period = result.period;
		assert(period >= 0);
		assert(period <= max_period);
		if(period == 0) {
			return 0;
		} else {
			return colors[period - 1];
		}
	} else {
		return result.escape_time > 100 ? 0 : 255;
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

pixel_t get_color(int i, int j) {
	if(points[i][j].has_value()) {
		if(points[i][j].value().escaped) {
			return points[i][j].value().escape_time > 100 ? 0 : 255; // todo
		} else {
			let period = points[i][j].value().period;
			assert(period >= 0);
			assert(period <= max_period);
			if(period == 0) {
				return 0;
			} else {
				return colors[period - 1];
			}
		}
	} else {
		assert(false);
		return {255, 0, 0};
	}
}

point_descriptor get_point(int i, int j) {
	// memoization logic
	if(points[i][j].has_value()) {
		return *points[i][j];
	} else {
		let [x, y] = get_coordinates(i, j);
		let m = mandelbrot(x, y);
		points[i][j] = m;
		return m;
	}
}

void brute_force_worker(std::atomic_int* xj, BMP* bmp, int id) {
	int j;
	while((j = xj->fetch_add(1, std::memory_order_relaxed)) < h) {
		if(id == 0) printf("\033[1K\r%0.2f%%", (fp)j / h * 100);
		if(id == 0) fflush(stdout);
		for(int i = 0; i < w; i++) {
			let [x, y] = get_coordinates(i, j);
			let color = sample(x, y);
			bmp->set(i, j, color);
		}
	}
}

void mariani_silver_worker(parallel_queue<std::tuple<int, int, int, int>>* _mq, gatekeeper* _gate) {
	auto T = std::tuple<parallel_queue<std::tuple<int, int, int, int>>&, gatekeeper&> { *_mq, *_gate };
	auto& [mq, gate] = T;
	while(true) {
		while(true) {
			mq.lock();
			//printf("%d\n", (int)mq.size());
			if(!mq.empty()) {
				let [i, j, w, h] = mq.pop();
				//printf("%d %d %d %d\n", i, j, w, h);
				mq.unlock();
				assert(w >= 0 && h >= 0);
				if(w <= 4 || h <= 4) {
					// an optimization but also handling an edge case where i + w/2 - 1 ==== i and cdiv(w, 2) + 1 ==== w
					for(int x = i; x < i + w; x++) {
						for(int y = j; y < j + h; y++) {
							points[x][y] = get_point(x, y);
						}
					}
					continue;
				}
				std::optional<point_descriptor> pd;
				bool all_same = true;
				for(int x = i; x < i + w; x++) {
					if(debug_info) ms_mask[x][j] = true;
					if(debug_info) ms_mask[x][j + h - 1] = true;
					let d1 = get_point(x, j);
					let d2 = get_point(x, j + h - 1);
					if(!pd.has_value()) pd = d1;
					if(*pd != d1) all_same = false;
					if(*pd != d2) all_same = false;
				}
				for(int y = j; y < j + h; y++) {
					if(debug_info) ms_mask[i][y] = true;
					if(debug_info) ms_mask[i + w - 1][y] = true;
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
					mq.lock();
					mq.push(std::tuple<int, int, int, int> {i,           j,           w / 2,          h / 2         });
					mq.push(std::tuple<int, int, int, int> {i + w/2 - 1, j,           cdiv(w, 2) + 1, h / 2         });
					mq.push(std::tuple<int, int, int, int> {i,           j + h/2 - 1, w / 2,          cdiv(h, 2) + 1});
					mq.push(std::tuple<int, int, int, int> {i + w/2 - 1, j + h/2 - 1, cdiv(w, 2) + 1, cdiv(h, 2) + 1});
					mq.unlock();
					gate.wake_idle_threads();
				}
			} else {
				mq.unlock();
				break;
			}
		}
		if(!gate.wait_for_more_work()) return;
	}
}

void AA_worker(BMP* _bmp, parallel_queue<std::pair<int, int>>* _aaq, std::mutex* _maskmutex, gatekeeper* _gate) {
	auto T = std::tuple<BMP&, parallel_queue<std::pair<int, int>>&,  std::mutex&, gatekeeper&> { *_bmp, *_aaq, *_maskmutex, *_gate };
	auto& [bmp, aaq, maskmutex, gate] = T;
	while(true) {
		while(true) {
			//printf("aaq: %d %s\n", (int)aaq.size(), aaq.debug_info().c_str());
			aaq.lock();
			if(!aaq.empty()) {
				let [i, j] = aaq.pop();
				aaq.unlock();
				let [x, y] = get_coordinates(i, j);
				let p = sample(x, y);
				if(p != bmp.get(i, j)) { // no lock needed for reading
					//let b = bmp.get(i, j);
					//printf("{%d, %d, %d} != {%d, %d, %d}\n", p.r, p.g, p.b, b.r, b.g, b.b);
					// no lock needed because only this thread should ever write to this pixel
					bmp.set(i, j, p);
					maskmutex.lock();
					for(int x = std::max(0, i - border_radius); x <= std::min(w - 1, i + border_radius); x++) {
						for(int y = std::max(0, j - border_radius); y <= std::min(h - 1, j + border_radius); y++) {
							if((x-i)*(x-i) + (y-j)*(y-j) > border_radius*border_radius) continue;
							bool do_add = false;
							if(!aa_mask[x][y]) {
								do_add = true;
								aa_mask[x][y] = true;
							}
							if(do_add) {
								aaq.atomic_push({x, y});
							}
						}
					}
					maskmutex.unlock();
				}
			} else {
				aaq.unlock();
				break;
			}
			//if(let s = aaq.atomic_size(); s % 200 == 0) {
			//	printf("--%d--\n", (int)s);
			//}
		}
		if(!gate.wait_for_more_work()) return;
	}
}

int main() {
	assert(byte_swap(0x11223344) == 0x44332211);
	assert(byte_swap(pixel_t{0x11, 0x22, 0x33}) == (pixel_t{0x33, 0x22, 0x11}));
	// Render pipeline:
	//   Mariani-silver figures out the mandelbrot main-body
	//   Color translation
	//   Edge detection ran to figure out where AA is needed
	//   AA pass, parallel
	//   Another pass to expose any missing detail as discovered by the AA pass
	BMP bmp = BMP(w, h);
	const int nthreads = 4; //std::thread::hardware_concurrency();
	printf("parallel on %d threads\n", nthreads);
	if(mode == render_mode::brute_force) {
		puts("starting brute force");
		std::vector<std::thread> vec(nthreads);
		std::atomic_int j = 0;
		for(int i = 0; i < nthreads; i++) {
			vec[i] = std::thread(brute_force_worker, &j, &bmp, i);
		}
		for(let& t : vec) {
			t.join();
		}
		puts("\033[1K\rfinished");
	} else {
		puts("starting mariani-silver");
		parallel_queue<std::tuple<int, int, int, int>> mq;
		mq.push({0, 0, w, h});
		// each thread will increment the counter when posting at the gate - exit condition is post == 0
		gatekeeper gate(-nthreads);
		std::vector<std::thread> thread_pool(nthreads);
		for(let& t : thread_pool) {
			t = std::thread(mariani_silver_worker, &mq, &gate);
		}
		for(let& t : thread_pool) {
			t.join();
		}
		puts("finished mariani-silver, starting color translation");
		// this could be folded into mariani-silver but it's super fast so it does not matter
		for(int i = 0; i < w; i++) {
			for(int j = 0; j < h; j++) {
				bmp.set(i, j, get_color(i, j));
			}
		}
		puts("finished color translation");
		if(AA) {
			puts("anti-alias enabled, starting anti-alias");
			parallel_queue<std::pair<int, int>> aaq;
			std::mutex maskmutex;
			gate.reset_to(-nthreads - 1);
			for(let& t : thread_pool) {
				t = std::thread(AA_worker, &bmp, &aaq, &maskmutex, &gate);
			}
			for(int i = 0; i < w; i++) {
				for(int j = 0; j < h; j++) {
					bool center = points[i][j].value().escaped;
					bool has_white = false;
					bool has_non_white = false;
					for(int x = std::max(0, i - 1); x <= std::min(w - 1, i + 1); x++) {
						for(int y = std::max(0, j - 1); y <= std::min(h - 1, j + 1); y++) {
							if(x == i && y == j) continue;
							if(points[x][y].value().escaped) {
								has_white = true;
							} else {
								has_non_white = true;
							}
							if(has_white && has_non_white) goto b;
						}
					}
					b:
					if((center && has_non_white) || (!center && has_white)) {
						maskmutex.lock(); // todo: note: very small critical section - put mutex outside of loop?
						for(int x = std::max(0, i - border_radius); x <= std::min(w - 1, i + border_radius); x++) {
							for(int y = std::max(0, j - border_radius); y <= std::min(h - 1, j + border_radius); y++) {
								if((x-i)*(x-i) + (y-j)*(y-j) > border_radius*border_radius) continue;
								bool do_add = false;
								if(!aa_mask[x][y]) {
									do_add = true;
									aa_mask[x][y] = true;
								}
								if(do_add) {
									aaq.atomic_push({x, y});
								}
							}
						}
						maskmutex.unlock();
					}
				}
				gate.wake_idle_threads(); // wake any threads which are waiting
			}
			gate.post();
			puts("main posted"); // fixme: debug
			for(let& t : thread_pool) {
				t.join();
			}
			puts("finished");
		}
		if(debug_info) {
			for(int i = 0; i < w; i++) {
				for(int j = 0; j < h; j++) {
					let [_r, _g, _b] = bmp.get(i, j);
					let [r, g, b, n] = std::tuple{(int)_r, (int)_g, (int)_b, 1};
					if(ms_mask[i][j]) { r += 255; g += 127; b += 38; n++; }
					if(aa_mask[i][j]) { r += 255; g += 0; b += 0; n++; }
					bmp.set(i, j, {(uint8_t)(r/n), (uint8_t)(g/n), (uint8_t)(b/n)});
				}
			}
		}
	}
	bmp.write("test.bmp");
}
