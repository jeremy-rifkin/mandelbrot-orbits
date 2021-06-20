#ifndef UTILS_H
#define UTILS_H

#include <assert.h>
#include <atomic>
#include <condition_variable>
#include <limits>
#include <math.h>
#include <mutex>
#include <queue>
#include <utility>
#include <unordered_set>
#include <vector>

#define let auto

constexpr bool is_little_endian() {
	let x = 1;
	return *(char*)&x;
}

template<typename T> T byte_swap(T t) {
	let* p = (uint8_t*)&t;
	for(std::size_t i = 0; i < sizeof(t) / 2; i++) {
		std::swap(p[i], p[sizeof(t) - 1 - i]);
	}
	return t;
}

// hash combining from https://stackoverflow.com/a/50978188/15675011

template<typename T> T xorshift(const T& n, int i) {
	return n^(n>>i);
}

inline uint32_t distribute(const uint32_t& n){
	uint32_t p = 0x55555555ul;
	uint32_t c = 3423571495ul;
	return c*xorshift(p*xorshift(n,16),16);
}

inline uint64_t hash(const uint64_t& n){
	uint64_t p = 0x5555555555555555;
	uint64_t c = 17316035218449499591ull;
	return c*xorshift(p*xorshift(n,32),32);
}

template <typename T,typename S>
typename std::enable_if<std::is_unsigned<T>::value,T>::type
constexpr rotl(const T n, const S i){
	const T m = (std::numeric_limits<T>::digits-1);
	const T c = i&m;
	return (n<<c)|(n>>((T(0)-c)&m));
}

inline std::size_t hash_combine(std::size_t&& h1, std::size_t&& h2) {
	return rotl(h1, std::numeric_limits<size_t>::digits / 3) ^ distribute(h2);
}

template<typename T> std::size_t count_unique(const std::vector<T>& vec) {
	// https://quick-bench.com/q/I3Z3LSivDA18VW_8r9ytw3liBRI
	if(vec.size() < 400) {
		std::size_t unique = 0;
		for(std::size_t i = 0; i < vec.size(); i++) {
			for(std::size_t j = i + 1; j < vec.size(); j++) {
				if(vec[i] == vec[j]) {
					goto next;
				}
			}
			unique++;
			next:;
		}
		return unique;
	} else {
		return std::unordered_set<T>(vec.begin(), vec.end()).size();
	}
}

// https://stackoverflow.com/questions/2353211/hsl-to-rgb-color-conversion
/**
 * Converts an HSL color value to RGB. Conversion formula
 * adapted from http://en.wikipedia.org/wiki/HSL_color_space.
 * Assumes s and l are contained in the set [0, 1] and h is [0, 360]
 * returns r, g, and b in the set [0, 255].
 *
 * @param   {number}  h       The hue
 * @param   {number}  s       The saturation
 * @param   {number}  l       The lightness
 * @return  {Array}           The RGB representation
 */
inline float hue2rgb(float p, float q, float t){
	if(t < 0) t += 1;
	if(t > 1) t -= 1;
	if(t < 1/6.) return p + (q - p) * 6 * t;
	if(t < 1/2.) return q;
	if(t < 2/3.) return p + (q - p) * (2/3. - t) * 6;
	return p;
}
inline std::tuple<uint8_t, uint8_t, uint8_t> hsl_to_rgb(float h, float s, float l){
	h /= 360;
	float r, g, b;
	if(s == 0) {
		r = g = b = l; // achromatic
	} else {
		float q = l < 0.5 ? l * (1 + s) : l + s - l * s;
		float p = 2 * l - q;
		r = hue2rgb(p, q, h + 1/3.);
		g = hue2rgb(p, q, h);
		b = hue2rgb(p, q, h - 1/3.);
	}
	return {round(r * 255), round(g * 255), round(b * 255)};
}

// ceiling division
template<typename T> T cdiv(T x, T y) {
	return (x + y - 1) / y;
}

/*
 * This is a parallel queue designed for multi-producer multi-consumer systems. The parameter `n`
 * Represents the number of producers + consumers. The queue will be populated with `n` optional
 * none entries when all producers and consumers finish.
 */
template<typename T> struct parallel_queue {
	std::queue<std::optional<T>> q;
	std::mutex m;
	std::condition_variable not_empty;
	const int initial_count;
	std::atomic_int count;
public:
	parallel_queue(int n) : initial_count(n), count(n) {};
	parallel_queue(const parallel_queue&) = delete;
	parallel_queue(parallel_queue&&) = delete;
	parallel_queue& operator=(const parallel_queue&) = delete;
	parallel_queue& operator=(parallel_queue&&) = delete;
	void lock() {
		m.lock();
	}
	void unlock() {
		m.unlock();
	}
	void unsync_push(T item) {
		q.push(item);
		not_empty.notify_one();
	}
	void push(T item) {
		m.lock();
		q.push(item);
		not_empty.notify_one();
		m.unlock();
	}
	[[nodiscard]] std::optional<T> pop() {
		std::unique_lock<std::mutex> u(m);
		if(q.empty()) {
			if(--count == 0) {
				for(int i = 0; i < initial_count; i++) {
					q.push({}); // fixme
				}
				not_empty.notify_all();
			} else {
				not_empty.wait(u, [&](){return !q.empty();});
				++count;
			}
		}
		assert(!q.empty());
		std::optional<T> r = std::move(q.front());
		q.pop();
		return r;
	}
	bool empty() {
		m.lock();
		bool e = q.empty();
		m.unlock();
		return e;
	}
	std::size_t size() {
		m.lock();
		std::size_t s = q.size();
		m.unlock();
		return s;
	}
	void extern_post() {
		std::unique_lock<std::mutex> u(m);
		if(--count == 0) {
			for(int i = 0; i < initial_count; i++) {
				q.push({}); // fixme
			}
			not_empty.notify_all();
		}
	}
};

template<typename T> struct atomic_optional {
	union {
		T item;
	};
	std::atomic_bool _has_value = false;
	atomic_optional() {}
	~atomic_optional() {
		if(_has_value) item.~T();
	}
	atomic_optional(const atomic_optional&) = delete;
	atomic_optional(atomic_optional&&) = delete;
	atomic_optional& operator=(const atomic_optional&) = delete;
	atomic_optional& operator=(atomic_optional&&) = delete;
	bool has_value() {
		return _has_value;
	}
	const T value() {
		return item;
	}
	const T operator*() {
		return item;
	}
	void operator=(T _item) {
		if(_has_value) item.~T();
		new (&item) T(_item);
		_has_value = true; // seq_cst
	}
};

#endif
