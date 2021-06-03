#ifndef UTILS_H
#define UTILS_H

#include <limits>
#include <utility>
#include <vector>
#include <unordered_set>

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

template<typename T> struct reverse_iter {
	decltype(std::declval<T>().rbegin()) _begin;
	decltype(std::declval<T>().rend()) _end;
	reverse_iter(T& iterable) : _begin(iterable.rbegin()), _end(iterable.rend()) {}
	decltype(auto) begin() { return _begin; }
	decltype(auto) end()   { return _end;   }
};

template<typename T> class enumerate {
	decltype(std::declval<T>().begin()) _begin;
	decltype(std::declval<T>().end()) _end;
public:
	enumerate(T&& iterable) : _begin(iterable.begin()), _end(iterable.end()) {}
	template<typename I> class iterator {
		I it;
		int i = 0;
	public:
		iterator(I it) : it(it) {}
		iterator operator++() { let i = *this; operator++(0); return i; }
		iterator operator++(int) { it++; i++; return *this; }
		std::pair<const int, decltype(*it)&> operator*() { return {i, *it}; }
		bool operator==(const iterator& o) { return it == o.it; }
		bool operator!=(const iterator& o) { return !operator==(o); }
	};
	template<typename I> class const_iterator {
		I it;
		int i = 0;
	public:
		const_iterator(I it) : it(it) {}
		const_iterator operator++() { let i = *this; operator++(0); return i; }
		const_iterator operator++(int) { it++; i++; return *this; }
		std::pair<const int, const decltype(*it)&> operator*() const { return {i, *it}; }
		bool operator==(const const_iterator& o) const { return it == o.it; }
		bool operator!=(const const_iterator& o) const { return !operator==(o); }
	};
	decltype(auto) begin()       { return iterator(_begin);       }
	decltype(auto) end()         { return iterator(_end);         }
	decltype(auto) begin() const { return const_iterator(_begin); }
	decltype(auto) end()   const { return const_iterator(_end);   }
};

#endif
