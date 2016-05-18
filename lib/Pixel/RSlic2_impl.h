#ifndef RSlic2_IMPL_H
#define RSlic2_IMPL_H

#include "RSlic2.h"
#include <priv/ZeroSlico_p.h>
#include <priv/Useful.h>
#include <3rd/ThreadPool.h>

#ifndef u_long
#define u_long unsigned long
#endif
#ifndef uint
#define uint unsigned int
#endif

using namespace RSlic::Pixel;
using RSlic::priv::aSize;

//Settings that are shared over several instances.
struct RSlic::Pixel::Slic2::Settings {
	Settings() : __refcount(0), step(0), stiffness(0) {
	}

	Settings(const Settings *other) :
			__refcount(0), step(other->step),
			img(other->img), stiffness(other->stiffness), pool(other->pool) {
	}

	void initThreadPool(int threadcount = -1) {
		if (threadcount <= 0)
			threadcount = std::thread::hardware_concurrency();
		pool = std::make_shared<ThreadPool>(threadcount);
	}

	Mat img;
	int step;
	int stiffness;
	shared_ptr<ThreadPool> pool;

	std::atomic<int> __refcount;
};


template<typename F>
RSlic::Pixel::Slic2P RSlic::Pixel::Slic2::iterate(F f) const {
	return iterate<F>(setting->stiffness, f);
}

namespace RSlic {
 namespace Pixel {
  namespace priv {

   /**
   * In order to share code between iterate and iterateZero we need
   * to take out the different parts.
   * This is the part from iterate.
   */
   template<typename F>
   struct DistNormal {
	   inline double operator()(const Vec2i &point, const Vec2i &center, int clusterIdx) {
		   return f(point, center, img, stiffness * stiffness, step);
	   }

	   const cv::Mat &img;
	   F &f;
	   int stiffness;
	   int step;
   };
  }
 }
}
namespace {
#ifdef PARALLEL

 /**
 * For doing some parallel computing we have to reduce the results of the threads.
 * Instead of iterating over all pixels, we just calculating the computed parts with
 * BRect.
 */
 struct BRect {
         static constexpr uint imax = std::numeric_limits<unsigned int>::max();
         struct point {
                 uint x;
                 uint y;
         };
         point topLeft, bottomRight;

         BRect(uint tx, uint ty, uint bx, uint by) : topLeft{tx, ty}, bottomRight{bx, by} {
         }

         BRect() : topLeft{imax, imax}, bottomRight{0, 0} {
         }

         void combineWith(const BRect &other) {
                 topLeft.x = std::min(topLeft.x, other.topLeft.x);
                 topLeft.y = std::min(topLeft.y, other.topLeft.y);
                 bottomRight.x = std::max(bottomRight.x, other.bottomRight.x);
                 bottomRight.y = std::max(bottomRight.y, other.bottomRight.y);
         }
 };

#endif
}
namespace RSlic {
 namespace Pixel {
  namespace priv {

   /**
   * Results of the common iteration algorithm
   */
   struct iterateCommonRes {
	   Mat_<ClusterInt> label;
	   Mat_<double> dist;

	   iterateCommonRes(int w, int h) : label(h, w, -1), dist(h, w, DINF) {
	   }

	   inline double &distAt(int y, int x) {
		   return dist.at<double>(y, x);
	   }

	   inline ClusterInt &labelAt(int y, int x) {
		   return label.at<ClusterInt>(y, x);
	   }

#ifdef PARALLEL
         BRect calcRect;
#endif
   };

   using iterateCommonResP = unique_ptr<iterateCommonRes>;
  }
 }
}
namespace {
 /**
 * Executes the Slic-Algorithm. Depending on the functor it computes the Slic or Slico version (or some unknown one ;).
 * @param f the functor. Have to be something like struct ExampleF{...; double operator()(const Vec2i& point, const Vec2i & center, int clusterIdx){...} ....}
 * @param beg the first cluster for computing
 * @param end the last cluster for computing
 * @param w the width of the picture
 * @param h the height of the picture
 * @param centers the central points of the clusters
 * @param s the step
 * @param pool the threadpool for parallel computing
 * @result the results composed of the label Mat, distance Mat and may the rect of calculation
 * @see iterate
 * @see iterateZero
 * @see priv::DistNormal
 */
 template<typename F>
 inline RSlic::Pixel::priv::iterateCommonResP iterateCommonIteration(F f, int beg, int end, int w, int h, const vector<Vec2i> &centers, int s, ThreadPoolP pool) {
	 RSlic::Pixel::priv::iterateCommonResP result(new RSlic::Pixel::priv::iterateCommonRes(w, h));
	 for (int k = beg; k < end; k++) {
		 auto center = centers[k];
		 int px = center[0];
		 int py = center[1];
#ifdef PARALLEL
                 BRect currentRect(std::max(0, px - s), std::max(0, py - s), std::min(w, px + s + 1), std::min(h, py + s + 1));
                 result->calcRect.combineWith(currentRect);
#endif
		 for (int x = std::max(0, px - s); x < std::min(w, px + s + 1); x++) {
			 for (int y = std::max(0, py - s); y < std::min(h, py + s + 1); y++) {
				 Vec2i point(x, y);
				 double &d = result->distAt(y, x);
				 double D = f(point, center, k);
				 if (D < d) {
					 d = D;
					 result->labelAt(y, x) = k;
				 }
			 }
		 }
	 }
	 return result;
 }

#ifndef PARALLEL

 /**
 * Executes the Slic-Algorithm. Depending on the functor it computes the Slic or Slico version (or some unknown one ;).
 * @param f the functor. Have to be something like struct ExampleF{...; double operator()(const Vec2i& point, const Vec2i & center, int clusterIdx){...} ....}
 * @param clusters the ClusterSet
 * @param s the step
 * @param pool the threadpool for parallel computing
 * @result the results composed of the label Mat and distance Mat
 * @see iterate
 * @see iterateZero
 * @see priv::DistNormal
 */
 template<typename F>
 RSlic::Pixel::priv::iterateCommonResP iterateCommon(F f, const ClusterSet &clusters, int s, ThreadPoolP pool) {
	 auto centers = clusters.getCenters();
	 int h = clusters.getClusterLabel().rows;
	 int w = clusters.getClusterLabel().cols;
	 int N = centers.size(); 

	 return iterateCommonIteration(f, 0, N, w, h, centers, s, pool);
 }

#else

/**
 * Executes the Slic-Algorithm. Depending on the functor it computes the Slic or Slico version (or some unknown one ;).
 * (Do the parallel computing version)
 * @param f the functor. Have to be something like struct ExampleF{...; double operator()(const Vec2i& point, const Vec2i & center, int clusterIdx){...} ....}
 * @param clusters the ClusterSet
 * @param s the step
 * @param pool the threadpool for parallel computing
 * @result the results composed of the label Mat and distance Mat
 * @see iterate
 * @see iterateZero
 * @see priv::DistNormal
 */
 template<typename F>
 RSlic::Pixel::priv::iterateCommonResP iterateCommon(F f, const ClusterSet &clusters, int s, ThreadPoolP pool) {
         auto centers = clusters.getCenters();
         int h = clusters.getClusterLabel().rows;
         int w = clusters.getClusterLabel().cols;
         RSlic::Pixel::priv::iterateCommonResP result(new  RSlic::Pixel::priv::iterateCommonRes(w, h));

         int N = centers.size(); 
         int thread_step = N / pool->threadcount();
         std::vector<std::future<RSlic::Pixel::priv::iterateCommonResP>> futures;
         futures.reserve(pool->threadcount());
         //Map
        
         for (int thread_start = 0; thread_start < N; thread_start += thread_step) {
                 futures.push_back(pool->enqueue([&](int start) {
                         return iterateCommonIteration(f, start, std::min(start + thread_step, N), w, h, centers, s, pool);
                 }, thread_start));
         }
         //Reduce
         for (auto &&fut: futures) {
                 auto &&thread_result = fut.get(); 
                 //Only use changed parts for reducing
                 for (int x = thread_result->calcRect.topLeft.x; x < thread_result->calcRect.bottomRight.x; x++) {
                         for (int y = thread_result->calcRect.topLeft.y; y < thread_result->calcRect.bottomRight.y; y++) {
                                 double dist_thread = thread_result->distAt(y, x);
                                 double &dist_result = result->distAt(y, x);
                                 if (dist_thread < dist_result) {
                                         dist_result = dist_thread;
                                         result->labelAt(y, x) = thread_result->labelAt(y, x);
                                 }
                         }
                 }
         }
         return result;
 }

#endif
}

template<typename F>
RSlic::Pixel::Slic2P RSlic::Pixel::Slic2::iterate(int stiffness, F f) const {
	int s = setting->step;
	int w = setting->img.cols;
	int h = setting->img.rows;

	// Setting up the normal Slic
	RSlic::Pixel::priv::DistNormal<F> distF{setting->img, f, stiffness, s};
	auto res = ::iterateCommon<RSlic::Pixel::priv::DistNormal<F>>(distF, clusters, s, setting->pool);

	// Creating the new instace
	Settings *newSetting = setting;
	if (setting->stiffness != stiffness) {
		newSetting = new Settings(setting);
		newSetting->stiffness = stiffness;
	}
	Slic2 *result = new Slic2(newSetting, ClusterSet(res->label, clusters.getCenters().size()), res->dist);

	return shared_ptr<Slic2>(result);
}

namespace {
 //Update the color-distance-maxima-matrix for slico that will be used for the next iteration.
 template<typename T>
 inline void iterateZeroUpdate(
		 const Mat &img, const Mat &label,
		 const vector<Vec2i> &centers, vector<double> &max_dist_color, shared_ptr<ThreadPool> pool) {
	 int w = img.cols;
	 int h = img.rows;
	 //Update Slico distance maxima
#ifdef PARALLEL
         int step = w / pool->threadcount();
         std::vector<std::future<void>> results;
         results.reserve(pool->threadcount());
         for (int xx = 0; xx < w; xx += step) {
                 results.push_back(pool->enqueue([&](int xx) {
                         for (int x = xx; x < std::min(xx + step, w); x++) {
#else
	 for (int x = 0; x < w; x++) {
#endif
		 for (int y = 0; y < h; y++) {
			 ClusterInt nearest_segment = label.at<ClusterInt>(y, x);
			 if (nearest_segment == -1) continue;
			 auto point = centers[nearest_segment];
			 int py = point[1];
			 int px = point[0];
             auto distColor = RSlic::priv::zero::zeroMetrik(img.at<T>(y, x), img.at<T>(py, px));
			 if (max_dist_color.at(nearest_segment) < distColor) {
				 max_dist_color.at(nearest_segment) = distColor;
			 }
		 }
	 }
#ifdef PARALLEL
                 }, xx));
         }
         for (auto &res: results) {res.get();}
#endif
 }

 //For calling without template
 declareCVF_T(iterateZeroUpdate, iterateZeroUpdateHelper,
		 return)
}
namespace RSlic {
 namespace Pixel {
  namespace priv {

   //Computing with zero
   template<typename F>
   struct DistZero {
	   inline double operator()(const Vec2i &point, const Vec2i &center, int clusterIdx) {
		   return f(point, center, img, max_distance[clusterIdx], step);
	   }

	   const cv::Mat &img;
	   F f;
	   const vector<double> &max_distance;
	   int step;
   };
  }
 }
}

//Slico
template<typename F>
RSlic::Pixel::Slic2P RSlic::Pixel::Slic2::iterateZero(F f) const {
	int s = setting->step;
	int w = setting->img.cols;
	int h = setting->img.rows;

	//Setting up Slico
	Pixel::priv::DistZero<F> distF{setting->img, f, max_dist_color, s};
	auto res = ::iterateCommon<Pixel::priv::DistZero<F>>(distF, clusters, s, setting->pool);

	auto newClusters = ClusterSet(res->label, clusters.getCenters().size());
	vector<double> new_max_dist_color(max_dist_color);
	//
	//Update values
	::iterateZeroUpdateHelper(setting->img.type(), setting->img, res->label, newClusters.getCenters(), new_max_dist_color, setting->pool);

	//Creating a new instance
	Slic2 *result = new Slic2(setting, std::move(newClusters), res->dist);
	result->max_dist_color = std::move(new_max_dist_color);
	return shared_ptr<Slic2>(result);
}

template<typename F>
RSlic::Pixel::Slic2P RSlic::Pixel::Slic2::finalize(F f) const {
	int w = setting->img.cols;
	int h = setting->img.rows;
	Mat_<ClusterInt> finalClusters(h, w, -1);
	int currentLabel = 0;
	const int lims = (h * w) / (clusters.clusterCount());
	static const int neighboursX[] = {1, 0, -1, 0};
	static const int neighboursY[aSize(neighboursX)] = {0, 1, 0, -1};

	for (int x = 0; x < w; x++) {
		for (int y = 0; y < h; y++) {
			//Some unassigned pixel?
			if (finalClusters.at<ClusterInt>(y, x) == -1) {
				vector<Vec2i> current_points;
				current_points.emplace_back(x, y);
				finalClusters.at<ClusterInt>(y, x) = currentLabel;

				//Look transitively for all unassigned neighbors
				//set them in current_points and finalCluster
				for (int i = 0; i < current_points.size(); i++) {
					Vec2i point = current_points[i];

					for (int neighbour = 0; neighbour < aSize(neighboursX); neighbour++) {
						int px = point[0] + neighboursX[neighbour];
						int py = point[1] + neighboursY[neighbour];
						if (px < 0 || px >= w || py < 0 || py >= h) continue; // not in the picture anymore
						if (finalClusters.at<ClusterInt>(py, px) == -1
								&& clusters.at(y, x) == clusters.at(py, px)) {
							current_points.emplace_back(px, py);
							finalClusters.at<ClusterInt>(py, px) = currentLabel;
						}
					}
				}

				//If there are not enough pixel in the cluster
				//look for the best in the environment and conjoin both
				if (current_points.size() <= lims >> 2) {
					int adjlabel = currentLabel; //best neighbor
					double topdist = DINF; //best neighbor value
					//finding best neighbor
					for (int neighbour = 0; neighbour < 4; neighbour++) {
						int px = x + neighboursX[neighbour];
						int py = y + neighboursY[neighbour];
						if (px < 0 || px >= w || py < 0 || py >= h) continue;
						ClusterInt label = finalClusters.at<ClusterInt>(py, px);
						if (label >= 0 && label != currentLabel) {
							double dist = f(Vec2i(x, y), Vec2i(px, py), setting->img, 1, setting->step);
							if (dist < topdist) { 
								topdist = dist;
								adjlabel = label;
							}
						}
					}
					//Set pixel to this neighbor
					for (const Vec2i point: current_points) {
						finalClusters.at<ClusterInt>(point[1], point[0]) = adjlabel;
					}
				} else currentLabel++; //Else I've created a new cluster
			}
		}
	}

	Slic2 *result = new Slic2(setting, ClusterSet(finalClusters, currentLabel), distance);
	return std::shared_ptr<Slic2>(result);
}


#endif // RSlic2_IMPL_H

