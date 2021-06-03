#ifndef KMEANS_H
#define KMEANS_H

#include <complex>
#include <vector>

struct cluster {
	std::complex<float> mean;
	std::vector<std::complex<float>> data;
};

std::vector<cluster> silhouette_find_clusters(const std::vector<std::complex<float>>&, int, int, int);

#endif
