#include "kmeans.h"

#include <algorithm>
#include <complex>
#include <random>
#include <unordered_set>
#include <vector>

#include "utils.h"

namespace std {
	template<typename T> struct hash<std::complex<T>> {
		std::size_t operator()(const std::complex<T>& c) const noexcept {
			return hash_combine(std::hash<T>()(c.real()), std::hash<T>()(c.imag()));
		}
	};
}

template<typename T> T average(const std::vector<T>& vec) {
	T v = 0;
	for(let item : vec) {
		v += item;
	}
	return v / (decltype(abs(T{0}))) vec.size();
}

int find_closest_cluster(const std::vector<cluster>& clusters, const std::complex<float> point) {
	let min_dist = abs(point - clusters[0].mean);
	let min_dist_i = 0;
	for(std::size_t j = 1; j < clusters.size(); j++) { // clusters.size() == k
		let d = abs(point - clusters[j].mean);
		if(d < min_dist) {
			min_dist = d;
			min_dist_i = j;
		}
	}
	return min_dist_i;
}

// k-means core - compute k-means for the given points
std::vector<cluster> kmeans(std::vector<std::complex<float>> data, std::size_t k, std::mt19937& rng) {
	// need at least k unique points
	/*#ifndef NDEBUG
	std::unordered_set<std::complex<float>> _s(data.begin(), data.end());
	assert(_s.size() >= (std::size_t) k);
	#endif
	// get k starting points
	std::shuffle(data.begin(), data.end(), rng);
	// // just use the first k iterations as starting points
	// use _s as a flag set for unused values
	std::vector<cluster> clusters;
	clusters.reserve(k);
	for(std::size_t i = 0; i < data.size() && clusters.size() < k; i++) {
		if(_s.count(data[i])) {
			clusters.push_back({data[i], {}});
			_s.erase(data[i]);
		}
	}
	assert(clusters.size() == k);*/
	// get k starting points
	std::vector<cluster> clusters(k);
	for(std::size_t i = 0; i < k; i++) {
		clusters[i].mean = data[i];
	}
	// iterate k-means
	int _counter = 1000; // prevent infinite loop
	bool did_move;
	do {
		did_move = false;
		// clear cluster data points from previous iteration
		for(let& cluster : clusters) cluster.data.clear();
		// assign points to their nearest clusters
		for(let const& point : data) {
			clusters[find_closest_cluster(clusters, point)].data.push_back(point);
		}
		// recalculate centroids
		for(let& cluster : clusters) {
			//assert(cluster.data.size() != 0);
			// FIXME
			if(cluster.data.size() == 0) {
				std::uniform_int_distribution U((decltype(data.size())) 0, data.size() - 1);
				cluster.mean = data[U(rng)];
				continue;
			}
			// FIXME
			let centroid = average(cluster.data);
			if(centroid != cluster.mean) {
				cluster.mean = centroid;
				did_move = true;
			}
		}
	} while(did_move && _counter-- > 0);
	if(_counter <= 0) {}//assert(false);
	// return final solution
	return clusters;
}

template<typename T> decltype(abs(T{0})) variation(const std::vector<T>& data) {
	// simple range can work well for k-means but there's not really a way to define the min/max
	// for complex numbers
	// maybe a bounding box?
	// anyway sum of squares is fine
	using fp = decltype(abs(T{0}));
	let mu = average(data);
	fp v = 0;
	for(let point : data) {
		v += pow(abs(point - mu), 2);
	}
	return v / (fp) data.size();
}

// find the max variation of a solution
template<typename T> decltype(auto) max_variation(const std::vector<T>& clusters) {
	let max = variation(clusters[0].data);
	for(std::size_t i = 1; i < clusters.size(); i++) {
		let v = variation(clusters[i].data);
		if(v > max) {
			max = v;
		}
	}
	return max;
}

// calculate the best k-means output of m runs
std::vector<cluster> kmeans_m(const std::vector<std::complex<float>>& data, int k, int m) {
	// run k-means m times
	std::mt19937 rng; // used across all runs
	let clusters = kmeans(data, k, rng);
	let current_best_var = max_variation(clusters);
	while(m---1) { // beautiful syntax isn't it! we already did 1 k-means computation so this loop needs to do m - 1 iterations
		let new_clusters = kmeans(data, k, rng);
		let variation = max_variation(new_clusters);
		if(variation < current_best_var) {
			current_best_var = variation;
			clusters = new_clusters;
		}
	}
	// return best solution of m runs
	return clusters;
}

// silhouette method for determining the number of clusters in a dataset (1d)
// compute a(i)
float silhouette_a(const cluster& c, std::complex<float> point) {
	let sum = 0.f;
	for(let pt : c.data) {
		// note: don't have to check the i!=j condition here since pt-pt is zero
		sum += abs(pt - point);
	}
	return 1.f / (c.data.size() - 1) * sum;
}
// compute b(i)
float silhouette_b(const std::vector<cluster>& clusters, std::size_t cluster_i, std::complex<float> point) {
	std::vector<float> mean_dists; // todo: minmax struct
	for(std::size_t i = 0; i < clusters.size(); i++) {
		if(i == cluster_i)
			continue;
		let sum = 0.f;
		for(let pt : clusters[i].data) {
			sum += abs(pt - point);
		}
		mean_dists.push_back(1.f / clusters[i].data.size() * sum);
	}
	return *std::min_element(mean_dists.begin(), mean_dists.end()); // no it's not elegant... TODO
}
// calculate the silhouette score of a data point
float silhouette_i(const std::vector<cluster>& clusters, int cluster_i, std::complex<float> point) {
	if(clusters[cluster_i].data.size() == 1) {
		return 0;
	}
	let a = silhouette_a(clusters[cluster_i], point),
		b = silhouette_b(clusters, cluster_i, point);
	return (b - a) / std::max(a, b);
}
// calculate the global silhouette score (an average of s(i) over all data points)
float silhouette(const std::vector<cluster>& clusters) {
	if(clusters.size() == 1) {
		return -1;
	}
	let sum = 0.f;
	let count = 0;
	for(std::size_t i = 0; i < clusters.size(); i++) {
		for(std::size_t j = 0; j < clusters[i].data.size(); j++) {
			sum += silhouette_i(clusters, i, clusters[i].data[j]);
			count++;
		}
	}
	return sum / count;
}

// find the optimal clustering
std::vector<cluster> silhouette_find_clusters(const std::vector<std::complex<float>>& data, int k_min, int k_max, int m) {
	let best_clusters = kmeans_m(data, k_min, m);
	let best_s = silhouette(best_clusters);
	k_max = std::min((std::size_t)k_max, count_unique(data));
	for(let k = k_min + 1; k <= k_max; k++) {
		let clusters = kmeans_m(data, k, m);
		let s = silhouette(clusters);
		if(s > best_s) {
			best_s = s;
			best_clusters = clusters;
		}
	}
	return best_clusters;
}
