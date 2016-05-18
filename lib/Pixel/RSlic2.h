#ifndef RSlic2_H
#define RSlic2_H

#include <stdint.h>
#include <string>
#include <functional>
#include <memory>
#include <mutex>
#include <opencv2/imgproc/imgproc.hpp>

#include "ClusterSet.h"

using namespace std;
using namespace cv;

class ThreadPool;

using ThreadPoolP=shared_ptr<ThreadPool>;


#define DINF std::numeric_limits<double>::infinity()

namespace RSlic {
 namespace Pixel {

  using DistanceFunc=function<double(const Vec2i & /*point*/, const Vec2i & /*clusterCenter*/, const Mat &/*img*/, int /*stiffness*/, int /*step*/)>;

  class Slic2;

  using Slic2P=shared_ptr<const Slic2>;

  class Slic2 {
  private:
	  struct Settings;

  public:

	  Slic2(const Slic2 &other) = delete;

	  Slic2(Slic2 &&other) = default;

	  /**
	  * initialize the algorithm. It build the gradient and set the needed values
	  * @param img the picture
	  * @param grad the gradient of the picture. Should be positive.
	  * @param step how many pixel should belongs (approximately) to a clusters
	  * @param stiffness the stiffness value
	  * @param pool ThreadPool for computing parallel.
	  * @return SharedPointer of the Slic2-Object. (Error -> nullptr)
	  * @see iterate
	  */
	  static Slic2P initialize(const Mat &img, const Mat &grad, int step, int stiffness, ThreadPoolP pool = ThreadPoolP());

	  ThreadPoolP threadpool() const;

	  /**
	  * Iterating the algorithm.
	  * @param f the functor with the metrics for the iteration.
	  * Has to be something like struct Example{..;inline double operator()(const cv::Vec2i &point, const cv::Vec2i &clusterCenter, const cv::Mat &mat, int stiffness, int step){...} ...}
	  * (stiffness will be passed squared)
	  * @return a new instance of Slic2 with the results of the iteration.
	  */
	  template<typename F>
	  Slic2P iterate(F f) const;

	  /**
	  * Iterating the algorithm with another stiffness.
	  * @param stiffness the stiffness factor.
	  * @param f the functor with the metrics for the iteration.
	  * @return a new instance of Slic2 with the results of the iteration.
	  * @see iterate
	  */
	  template<typename F>
	  Slic2P iterate(int stiffness, F f) const;

	  /**
	  * Iterating the algorithm
	  * using the zero parameter version of the SLIC algorithm (SLICO)
	  * @param f the functor with the metrics for the iteration
	  * @return a new instance of Slic2 with the results of the iteration.
	  */
	  template<typename F>
	  Slic2P iterateZero(F f) const;


	  /**
	  * Enforce connectivity.
	  * Often this is the last step you want to do.
	  * But you can do another iteration after here.
	  * Even if it does not make any sense.
	  * @param f the functor with the metrics for the iteration
	  * @return a new instance of Slic2 with the results of the iteration.
	  */
	  template<typename F>
	  Slic2P finalize(F f) const;

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
	  * Returns the image
	  * @return the image
	  */
	  Mat getImg() const;

	  /**
	  * Returns the computed clusters.
	  * @return computed clusters
	  */
	  const ClusterSet &getClusters() const;

	  virtual ~Slic2();

  private:
	  ClusterSet clusters;
	  Settings *setting;
	  Mat distance;

	  vector<double> max_dist_color; // For Slico (square values)
  protected:
	  Slic2(Settings *s, ClusterSet &&clusters = ClusterSet(), const Mat &distance = cv::Mat());

	  void init(const Mat &grad);
  };

 }
}
#endif // RSlic2_H
