#include "ClusterSet.h"
#include "../priv/Useful.h"

using namespace RSlic::Voxel;
using RSlic::priv::aSize;

inline tuple<u_long, u_long, u_long> operator+(const tuple<u_long, u_long, u_long> &a, const tuple<u_long, u_long, u_long> &b) {
	return make_tuple(
			std::get<0>(a) + std::get<0>(b),
			std::get<1>(a) + std::get<1>(b),
			std::get<2>(a) + std::get<2>(b)
	);
}

RSlic::Voxel::ClusterSet3::ClusterSet3(cv::Mat_<ClusterInt> clusters, int clusterCount)
		: data{clusters, vector<Vec3i>(), clusterCount, false} {
}

Mat_<ClusterInt> RSlic::Voxel::ClusterSet3::getClusterLabel() const {
	return data.clusterLabel;
}

const vector<Vec3i> &RSlic::Voxel::ClusterSet3::getCenters() const {
	if (!data.centers_calculated) refindCenters();
	return data.centers;
}

int RSlic::Voxel::ClusterSet3::clusterCount() const {
	return data._clusterCount;
}

void RSlic::Voxel::ClusterSet3::refindCenters() const {
	std::lock_guard<std::mutex> guard(mutex);
	if (data.centers_calculated) return;
	int w = data.clusterLabel.size[1];
	int h = data.clusterLabel.size[0];
	int d = data.clusterLabel.size[2];
	vector<u_long> centersCounts(data._clusterCount, 0);
	vector<tuple<u_long, u_long, u_long>> centerCoord(data._clusterCount, make_tuple(0l, 0l, 0l));

	for (int x = 0; x < w; x++) {
		for (int y = 0; y < h; y++) {
			for (int t = 0; t < d; t++) {
				ClusterInt idx = data.clusterLabel.at<ClusterInt>(y, x, t);
				if (idx < 0) continue;
				centersCounts[idx] = centersCounts[idx] + 1;
				centerCoord[idx] = centerCoord[idx] + make_tuple(static_cast<u_long>(x), static_cast<u_long>(y), static_cast<u_long>(t));
			}
		}
	}

	for (int i = 0; i < data._clusterCount; i++) {
		auto counts = centersCounts[i];
		assert(counts > 0);
		/*if (counts == 0) {
			centers.push_back(Vec3i(0, 0, 0));
			continue;
		}*/
		data.centers.emplace_back(
				std::get<0>(centerCoord[i]) / counts,
				std::get<1>(centerCoord[i]) / counts,
				std::get<2>(centerCoord[i]) / counts);
	}
	data.centers_calculated = true;
}

Mat RSlic::Voxel::ClusterSet3::maskOfCluster(ClusterInt idx) const {
	int w = data.clusterLabel.size[1];
	int h = data.clusterLabel.size[0];
	int d = data.clusterLabel.size[2];
	cv::Mat res = cv::Mat(3, data.clusterLabel.size, CV_8U);
	for (int x = 0; x < w; x++) {
		for (int y = 0; y < h; y++) {
			for (int t = 0; t < d; t++) {
				if (data.clusterLabel.at<ClusterInt>(y, x, t) == idx)
					res.at<uint8_t>(y, x, t) = 1;
			}
		}
	}
	return res;
}


namespace {
 inline void setClusterAdjacent(Mat &m, int p1, int p2) {
	 m.at<uint8_t>(p1, p2) = 1;
	 m.at<uint8_t>(p2, p1) = 1;
 }
}

cv::Mat RSlic::Voxel::ClusterSet3::adjacentMatrix() const {
	int size = clusterCount();
	Mat res = Mat::eye(size, size, CV_8UC1);
	Mat_<ClusterInt> clusterMat = getClusterLabel();
	int w = clusterMat.size[1];
	int h = clusterMat.size[0];
	int d = clusterMat.size[2];
	static const int xNeighbour[] = {1, 0, 0, 1, 0, 1, 1};
	static const int yNeighbour[aSize(xNeighbour)] = {0, 1, 0, 1, 1, 0, 1};
	static const int zNeighbour[aSize(xNeighbour)] = {0, 0, 1, 0, 1, 1, 1};
	for (int x = 0; x < w - 1; x++) {
		for (int y = 0; y < h - 1; y++) {
			for (int t = 0; t < d - 1; t++) {
				ClusterInt currentCluster = clusterMat.at<RSlic::Voxel::ClusterInt>(y, x, t);
				for (int i = 0; i < aSize(xNeighbour); i++) {
					ClusterInt otherCluster = clusterMat.at<RSlic::Voxel::ClusterInt>(y + yNeighbour[i], x + xNeighbour[i], t + zNeighbour[i]);
					if (currentCluster != otherCluster) {
						setClusterAdjacent(res, currentCluster, otherCluster);
					}
				}
			}
		}
	}
	return res;
}
