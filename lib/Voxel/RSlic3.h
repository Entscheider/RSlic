#ifndef RSlic3_H
#define RSlic3_H

#include <stdint.h>
#include <string>
#include <functional>
#include <memory>
#include <opencv2/imgproc/imgproc.hpp>
#include <ostream>
#include <thread>
#include <mutex>

#include "ClusterSet.h"



using namespace std;
using namespace cv;

class ThreadPool;

using ThreadPoolP=shared_ptr<ThreadPool>;

#define DINF std::numeric_limits<double>::infinity()

namespace RSlic {
 namespace Voxel {

  /**
  * Abstract class for managing the pictures of a sequence for the Supervoxel algorithm.
  * (E.g. load and destroy images if memory is low)
  * By subclassing it you have to make sure that are pictures have the same type and size.
  */
  class MovieCache {
  public:

	  /**
	  * Returns the image at the position t
	  * @param t the position of the image
	  * @return the image as a Mat. (Empty Mat for an invalid t)
	  */
	  virtual Mat matAt(int t) const = 0;

	  /**
	  * Returns the value of the point x,y from the picture at position t
	  * (Similar to Mat::at)
	  * @param y  y coordinate
	  * @param x x coordinate
	  * @param t position of the image
	  * @return value of the point
	  */
	  template<typename T>
	  inline T at(int y, int x, int t) const {
		  return matAt(t).at<T>(y, x);
	  }

	  /**
	  * Returns the value of the point v
	  * (Equally to MovieCacheP::at(v[1],v[0],v[3])=
	  * @param v the point
	  * @return value of the point
	  */
	  template<typename T>
	  inline T at(const Vec3i &v) const {
		  return matAt(v[2]).at<T>(v[1], v[0]);
	  }

	  /**
	  * Returns the duration (number of images of the sequence)
	  * @return the duration
	  */
	  virtual int duration() const = 0;

	  /**
	  * Returns the width of *all* images
	  * @return the width
	  */
	  inline int width() const {
		  return matAt(0).size[1];
	  }

	  /**
	  * Returns the width of *all* images
	  * @return the height
	  */
	  inline int height() const {
		  return matAt(0).size[0];
	  }

	  /**
	  * Returns the height, width and durations as a tuple
	  * @returns size tuple
	  */
	  inline tuple<int, int, int> sizeTuple() const {
		  return std::make_tuple(matAt(0).size[0], matAt(0).size[1], duration());
	  }


	  /**
	  * Returns the height, width and durations as an array.
	  * The array have to be deleted.
	  * @returns size array
	  */
	  inline int *sizeArray() const {
		  int *res = new int[3];
		  res[0] = matAt(0).size[0];
		  res[1] = matAt(0).size[1];
		  res[2] = duration();
		  return res;
	  }

	  /**
	  * Returns the type of *all* images
	  * @returns image type
	  */
	  inline int type() const {
		  return matAt(0).type();
	  }
  };

  using  MovieCacheP=shared_ptr<MovieCache>;



  using DistanceFunc = function<double(const Vec3i & /*point*/, const Vec3i & /*clusterCenter*/, const MovieCacheP &/*img*/, int /*stiffness*/, int /*step*/)>;
  using GradFunc = function<double(const MovieCacheP &, const Vec3i &)>;

  class Slic3;

  using Slic3P = shared_ptr<Slic3>;

  class Slic3 {
  private:
	  struct Settings;

  public:
	  Slic3(const Slic3 &other) = delete;

	  Slic3(Slic3 &&other) = default;

	  /**
	  * initialize the algorithm. It build the gradient and set the needed values
	  * @param img the MoveCache
	  * @param a function to calculate the gradient
	  * @param step how many pixel should belongs (approximately) to a clusters
	  * @param stiffness the stiffness value
	  * @param pool ThreadPool for computing parallel.
	  * @return SharedPointer of the Slic3-Object. (Error -> nullptr)
	  * @see iterate
	  */
	  static Slic3P initialize(const MovieCacheP &img, const GradFunc &grad, int step, int stiffness, ThreadPoolP pool = ThreadPoolP());


	  /**
	  * Iterating the algorithm.
	  * @param f the functor with the metrics for the iteration.
	  * Has to be something like struct Example{..; inline double operator()(const cv::Vec3i &point, const cv::Vec3i &clusterCenter, const MovieCacheP &mat, int stiffness, int step){...} ...}
	  * (stiffness will be passed squared)
	  * @return a new instance of Slic3 with the results of the iteration.
	  */
	  template<typename F>
	  Slic3P iterate(F f) const;

	  /**
	  * Iterating the algorithm with another stiffness.
	  * @param stiffness the stiffness factor.
	  * @param f the functor with the metrics for the iteration.
	  * @return a new instance of Slic3 with the results of the iteration.
	  * @see iterate
	  */
	  template<typename F>
	  Slic3P iterate(int stiffness, F f) const; 

	  ThreadPoolP threadpool() const;

	  /**
	  * Iterating the algorithm
	  * using the zero parameter version of the SLIC algorithm (SLICO)
	  * @param f the functor with the metrics for the iteration
	  * @return a new instance of Slic3 with the results of the iteration.
	  */
	  template<typename F>
	  Slic3P iterateZero(F f) const;

	  /**
	  * Enforce connectivity.
	  * Often this is the last step you want to do.
	  * But you can do another iteration after here.
	  * Even if it does not make any sense.
	  * @param f the functor with the metrics for the iteration
	  * @return a new instance of Slic3 with the results of the iteration.
	  */
	  template<typename F>
	  Slic3P finalize(F f) const;

	  /**
	  * Returns the step value
	  * @return step value
	  */
	  int getStep() const;

	  /**
	  * Returns the stiffness value
	  * @return stiffness value
	  */
	  int getStiffness() const;

	  /**
	  * Returns the MovieCache
	  * @return the MovieCache
	  */
	  MovieCacheP getImg() const;

	  /**
	  * Returns the computed clusters.
	  * @return computed clusters
	  */
	  const ClusterSet3 &getClusters() const;

	  virtual ~Slic3();


  private:
	  ClusterSet3 clusters;
	  Settings *setting;
	  Mat distance; //3-dim
	  double error;

	  vector<double> max_dist_color; 
  protected:
	  Slic3(Settings *s, ClusterSet3 &&set = ClusterSet3(), const Mat &distance = Mat());

	  void init();
  };
 }
}
#endif // RSlic3_H
