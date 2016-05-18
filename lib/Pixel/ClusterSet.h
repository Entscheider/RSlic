#ifndef CLUSTERSET2_H
#define CLUSTERSET2_H
#include <vector>
#include <opencv2/core/core.hpp>

using namespace std;
using namespace cv;

namespace RSlic{
namespace Pixel{
using ClusterInt = int16_t;

/**
* @brief Representing cluster (also called "Superpixel") with some functionality
*/
 class ClusterSet {
 public:
     /**
     * Constructor for an empty ClusterSet
     */
     ClusterSet() : data{cv::Mat(), std::vector<Vec2i>(), 0, true, false, cv::Mat()} {
     }

     ClusterSet(const ClusterSet &other) : data(other.data) {
     }

     ClusterSet(ClusterSet &&other) : data(std::move(other.data)) {
     }

     RSlic::Pixel::ClusterSet &operator=(ClusterSet &&other) {
         data = std::move(other.data);
         return *this;
     }

     RSlic::Pixel::ClusterSet &operator=(const ClusterSet &other) {
         data = other.data;
         return *this;
     }

     /**
     * Initialize with given centers and cluster-label mat
     * @param _centers List with the central points of the clusters
     * @param _clusters Mat where _clusters[i,j]=x means that the point i,j belongs to the cluster number x
     */
     template<typename T, typename= typename std::enable_if<
             std::is_same<vector<Vec2i>, typename std::decay<T>::type>::value
     >::type>
     ClusterSet(T &&_centers, cv::Mat_<ClusterInt> _clusters)
             : data{_clusters, std::forward<T>(_centers), 0, true, false, cv::Mat()} {
         data._clusterCount = _centers.size();
     }

     /**
     * Initialize with given clusters and the cluster's amount.
     * The central points will be calculating if necessary.
     * @param clusters Mat where clusters[i,j]=x means that the point i,j belongs to the cluster number x
     * @param clusterCount the amount of the clusters
     * @see getCenters()
     */
     ClusterSet(cv::Mat_<ClusterInt> clusters, int clusterCount);

     /**
     * Returns a mask of the cluster with the number idx.
     * @param idx Clusters index
     * @return binary mask.
     */
     Mat maskOfCluster(ClusterInt idx) const; 

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
     const vector<Vec2i> &getCenters() const;

     /**
     * Returns the amount of clusters.
     * @return amount of clusters
     */
     int clusterCount() const noexcept;

     /**
     * Returns the index of the cluster the point x,y belongs to.
     * @param y y-coordinate
     * @param x x-coordinate
     * @return cluster index
     */
     inline ClusterInt at(int y, int x) const {
         return data.clusterLabel.at<ClusterInt>(y, x);
     }

     /**
     * Returns the adjacent matrix m (type CV_8UC1).
     * If m[x,y] == 1 then the cluster with number x
     * and the cluster with number y are neighbour.
     * It is always m[x,x] == 1 and if m[x,y]==1
     * it means that m[y,x] == 1.
     * @return adjacent matrix.
     */
     const cv::Mat adjacentMatrix() const;

 private:
     struct nonspecial { //for default Copy & Move Construcors
         nonspecial(nonspecial &&other) = default;
         nonspecial(const nonspecial &other) = default;
         nonspecial& operator=(const nonspecial & other) = default;
         nonspecial& operator=(nonspecial && other) = default;

         Mat_<ClusterInt> clusterLabel;

         mutable vector<Vec2i> centers;
         int _clusterCount;
         mutable bool centers_calculated, adj_calculated;
         mutable Mat_<uint8_t> adjMatrix;
     } data;
     mutable std::mutex centerMutex, adjMutex;

     void refindCenters() const; //computes central points -> data.centers
     void refindAdjacent() const; //computes adjacent matrix -> data.adjMatrix
 };
}
}
#endif // CLUSTERSET_H
