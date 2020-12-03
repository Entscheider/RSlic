#include "ClusterSet.h"
#include "../priv/Useful.h"

using namespace RSlic::Pixel;
using RSlic::priv::aSize;

RSlic::Pixel::ClusterSet::ClusterSet(cv::Mat_<ClusterInt> clusters, int clusterCount)
		: data{clusters, std::vector<Vec2i>(), clusterCount, false} {
}

Mat_<ClusterInt> RSlic::Pixel::ClusterSet::getClusterLabel() const {
	return data.clusterLabel;
}

const vector<Vec2i> &RSlic::Pixel::ClusterSet::getCenters() const {
	if (!data.centers_calculated) {
		refindCenters();
	}
	return data.centers;
}

int RSlic::Pixel::ClusterSet::clusterCount() const noexcept {
	return data._clusterCount;
}

namespace {
 inline void setClusterAdjacent(Mat &m, int p1, int p2) {
	 m.at<uint8_t>(p1, p2) = 1;
	 m.at<uint8_t>(p2, p1) = 1;
 }
}

const cv::Mat RSlic::Pixel::ClusterSet::adjacentMatrix() const {
	if (!data.adj_calculated) {
		refindAdjacent();
	}
	return data.adjMatrix;
}

void RSlic::Pixel::ClusterSet::refindAdjacent() const {
	std::lock_guard<std::mutex> guard(adjMutex);
	if (data.adj_calculated) return;
	int size = clusterCount();
	Mat res = Mat::eye(size, size, CV_8UC1);
	auto clusterMat = getClusterLabel();
	int w = clusterMat.cols;
	int h = clusterMat.rows;
	static const int xNeighbour[] = {1, 0, 1};
	static const int yNeighbour[aSize(xNeighbour)] = {0, 1, 1};
	for (int x = 0; x < w - 1; x++) {
		for (int y = 0; y < h - 1; y++) {
			ClusterInt currentCluster = clusterMat.at<ClusterInt>(y, x);
			for (int i = 0; i < aSize(xNeighbour); i++) {
				ClusterInt otherCluster = clusterMat.at<ClusterInt>(y + yNeighbour[i], x + xNeighbour[i]);
				if (currentCluster != otherCluster) {
					setClusterAdjacent(res, currentCluster, otherCluster);
				}
			}
		}
	}
	data.adjMatrix = res;
}

inline tuple<u_long, u_long> operator+(const tuple<u_long, u_long> &a, const tuple<u_long, u_long> &b) {
	return make_tuple(std::get<0>(a) + std::get<0>(b), std::get<1>(a) + std::get<1>(b));
}

void RSlic::Pixel::ClusterSet::refindCenters() const {
	std::lock_guard<std::mutex> guard(centerMutex);
	if (data.centers_calculated) return;

	vector<u_long> centersCounts(data._clusterCount, 0);
	vector<tuple<u_long, u_long>> centerCoord(data._clusterCount, make_tuple(0l, 0l));//Da Werte länger als sizeof(int) sein kann


	for (int x = 0; x < data.clusterLabel.cols; x++) {
		for (int y = 0; y < data.clusterLabel.rows; y++) {
			ClusterInt idx = data.clusterLabel.at<ClusterInt>(y, x);
			if (idx < 0) continue;
			centersCounts[idx]++;
			centerCoord[idx] = centerCoord[idx] + make_tuple(static_cast<u_long>(x), static_cast<u_long>(y));
		}
	}


	for (int i = 0; i < data._clusterCount; i++) {
		auto counts = centersCounts[i];
		if (counts == 0) {
			data.centers.emplace_back(0, 0);
			continue;
		}
		data.centers.emplace_back(std::get<0>(centerCoord[i]) * 1.0 / counts, std::get<1>(centerCoord[i]) * 1.0 / counts);
	}
	data.centers_calculated = true;
}

Mat RSlic::Pixel::ClusterSet::maskOfCluster(ClusterInt idx) const {
	int h = data.clusterLabel.rows;
	int w = data.clusterLabel.cols;
	cv::Mat res = cv::Mat::zeros(h, w, CV_8U);
	for (int x = 0; x < w; x++) {
		for (int y = 0; y < h; y++) {
			if (data.clusterLabel.at<ClusterInt>(y, x) == idx)
				res.at<uint8_t>(y, x) = 1;
		}
	}
	return res;
}

