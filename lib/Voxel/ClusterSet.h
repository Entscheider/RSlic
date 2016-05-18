#ifndef ClUSTERSET3_H
#define ClUSTERSET3_H

#include <vector>
#include <opencv2/core/core.hpp>

using namespace std;
using namespace cv;

namespace RSlic {
 namespace Voxel {
  using ClusterInt=int; //int32_t

/**
* @brief Representing cluster (also called "Supervoxel") with some functionality
*/
  class ClusterSet3 {
  public:

	  ClusterSet3() : data{Mat_<ClusterInt>(), vector<Vec3i>(), 0, false} {
	  }

	  ClusterSet3(ClusterSet3 &&other) : data(std::move(other.data)) {
	  }

	  ClusterSet3(const ClusterSet3 &other) : data(other.data) {
	  }

	  ClusterSet3 &operator=(ClusterSet3 &&other) {
		  data = std::move(other.data);
		  return *this;
	  }

	  ClusterSet3 &operator=(const ClusterSet3 &other) {
		  data = other.data;
		  return *this;
	  }

	  /**
	  * Initialize with given centers and cluster-label mat
	  * @param _centers List with the central points of the clusters
	  * @param _clusters Mat where _clusters[i,j]=x means that the point i,j belongs to the cluster number x
	  */
	  template<typename T, typename= typename std::enable_if<
			  std::is_same<vector<Vec3i>, typename std::decay<T>::type>::value
	  >::type>
	  ClusterSet3(T &&_centers, cv::Mat_<ClusterInt> _clusters) : data{_clusters, std::forward<T>(_centers), 0, true} {
		  data._clusterCount = data.centers.size();
	  }

	  /**
	  * Initialize with given clusters and the cluster's amount.
	  * The central points will be calculating if necessary.
	  * @param clusters Mat where clusters[i,j]=x means that the point i,j belongs to the cluster number x
	  * @param clusterCount the amount of the clusters
	  * @see getCenters()
	  */
	  ClusterSet3(cv::Mat_<ClusterInt> clusters, int clusterCount);


	  /**
	  * Returns a mask of the cluster with the number idx.
	  * @param idx Clusters index
	  * @return binary mask.
	  */
	  Mat maskOfCluster(ClusterInt idx) const; // Binäres Bild. 0 => gehört nicht dazu, 1 => gehört dazu


	  /**
	  * Returns a Mat m where where m[x,y]=i means that the point x,y belongs to the cluster with the number i
	  * @return Mat with the cluster label
	  */
	  Mat_<ClusterInt> getClusterLabel() const;

	  /**
	  * Returns the central points of the clusters.
	  * The index indicates the cluster number the central point belongs to.
	  * This is a lazy-evaluation.
	  * @return List of central points
	  */
	  const vector<Vec3i> &getCenters() const;

	  /**
	  * Returns the amount of clusters.
	  * @return amount of clusters
	  */
	  int clusterCount() const;

	  /**
	  * Returns the index of the cluster the point x,y belongs to.
	  * @param y y-coordinate
	  * @param x x-coordinate
	  * @return cluster index
	  */
	  inline ClusterInt at(int y, int x, int t) const {
		  return data.clusterLabel.at<ClusterInt>(y, x, t);
	  }

	  /**
	  * Returns the adjacent matrix m (type CV_8UC1).
	  * If m[x,y] == 1 then the cluster with number x
	  * and the cluster with number y are neighbor.
	  * It is always m[x,x] == 1 and if m[x,y]==1
	  * it means that m[y,x] == 1.
	  * @return adjacent matrix.
	  */
	  cv::Mat adjacentMatrix() const;

  private:
	  struct nonspecial { //For default copy & move constructor
		  nonspecial(nonspecial &&other) = default;

		  nonspecial(const nonspecial &other) = default;

		  nonspecial &operator=(const nonspecial &other) = default;

		  nonspecial &operator=(nonspecial &&other) = default;

		  Mat_<ClusterInt> clusterLabel; //3-Dim

		  mutable vector<Vec3i> centers;
		  int _clusterCount;
		  mutable bool centers_calculated;
	  } data;

	  mutable std::mutex mutex;

	  void refindCenters() const;
  };

 }
}
#endif
