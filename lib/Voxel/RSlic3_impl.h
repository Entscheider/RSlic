#ifndef RSlic3_IMPL_H
#define RSlic3_IMPL_H

#include "RSlic3.h"
#include <priv/ZeroSlico_p.h>
#include <priv/Useful.h>
#include <3rd/ThreadPool.h>

#ifndef u_long
#define u_long unsigned long
#endif
#ifndef uint
#define uint unsigned int
#endif

using RSlic::priv::aSize;
using namespace RSlic::Voxel;


struct RSlic::Voxel::Slic3::Settings {
	Settings() : __refcount(0), step(0), stiffness(0) {
	}

	Settings(const Settings *other) :
			__refcount(0), step(other->step), distFunc(other->distFunc), pool(other->pool),
			img(other->img), stiffness(other->stiffness), gradFunc(other->gradFunc) {
	}

	void initThread(int threadcount = -1) {
		if (threadcount <= 0)
			threadcount = std::thread::hardware_concurrency();
		pool = std::make_shared<ThreadPool>(threadcount);
	}

	~Settings() {
	}

	MovieCacheP img;
	int step;
	int stiffness;
	shared_ptr<ThreadPool> pool;
	DistanceFunc distFunc;
	GradFunc gradFunc;

	std::atomic<int> __refcount;
};

template<typename F>
Slic3P RSlic::Voxel::Slic3::iterate(F f) const {
	return iterate(setting->stiffness, f);
}

namespace RSlic {
 namespace Voxel {
  namespace priv {
/* See RSlic2_impl.h for more information
 */
   template<typename F>
   struct DistNormal {
	   inline double operator()(const Vec3i &point, const Vec3i &center, int clusterIdx) {
		   return f(point, center, img, stiffness * stiffness, step);
	   }

	   const RSlic::Voxel::MovieCacheP img;
	   F f;
	   int stiffness;
	   int step;
   };
  }
 }
}
namespace {
 /*
 	See RSlic2_impl.h for more information
  */
 struct BRect {
	 static constexpr uint imax = std::numeric_limits<unsigned int>::max();
	 struct point {
		 uint x;
		 uint y;
		 uint z;
	 };
	 point topFrontLeft, bottomBackRight;

	 BRect(uint tx, uint ty, uint tz, uint bx, uint by, uint bz)
			 : topFrontLeft{tx, ty, tz}, bottomBackRight{bx, by, bz} {
	 }

	 BRect() : topFrontLeft{imax, imax, imax}, bottomBackRight{0, 0, 0} {
	 }

	 void combineWith(const BRect &other) {
		 topFrontLeft.x = std::min(topFrontLeft.x, other.topFrontLeft.x);
		 topFrontLeft.y = std::min(topFrontLeft.y, other.topFrontLeft.y);
		 topFrontLeft.z = std::min(topFrontLeft.z, other.topFrontLeft.z);
		 bottomBackRight.x = std::max(bottomBackRight.x, other.bottomBackRight.x);
		 bottomBackRight.y = std::max(bottomBackRight.y, other.bottomBackRight.y);
		 bottomBackRight.z = std::max(bottomBackRight.z, other.bottomBackRight.z);
	 }
 };
}
namespace RSlic {
 namespace Voxel {
  namespace priv {
   struct iterateCommonRes {
	   Mat_<ClusterInt> label;
	   Mat_<double> dist;

	   iterateCommonRes(const cv::MatSize &size) : label(3, size, -1), dist(3, size, DINF) {
	   }

	   inline double &distAt(int y, int x, int t) {
		   return dist.at<double>(y, x, t);
	   }

	   inline ClusterInt &labelAt(int y, int x, int t) {
		   return label.at<ClusterInt>(y, x, t);
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
 template<typename F>
 inline RSlic::Voxel::priv::iterateCommonResP iterateCommonIteration(F f, int beg, int end, const cv::MatSize &size, const vector<Vec3i> &centers, int s, ThreadPoolP pool) {
	 RSlic::Voxel::priv::iterateCommonResP result(new RSlic::Voxel::priv::iterateCommonRes(size));
	 const int w = size[1];
	 const int h = size[0];
	 const int duration = size[2];
	 for (int k = beg; k < end; k++) {
		 auto center = centers[k];
		 int px = center[0];
		 int py = center[1];
		 int pt = center[2];
#ifdef PARALLEL
                 BRect currentRect(std::max(0, px - s), std::max(0, py - s), std::max(0, pt - s),
                                 std::min(w, px + s + 1), std::min(h, py + s + 1), std::min(duration, pt + s + 1));
                 result->calcRect.combineWith(currentRect);
#endif
		 for (int x = std::max(0, px - s); x < std::min(w, px + s + 1); x++) {
			 for (int y = std::max(0, py - s); y < std::min(h, py + s + 1); y++) {
				 for (int t = std::max(0, pt - s); t < std::min(duration, pt + s + 1); t++) {
					 Vec3i point(x, y, t);
					 double &d = result->distAt(y, x, t);
					 double D = f(point, center, k);
					 if (D < d) {
						 d = D;
						 result->labelAt(y, x, t) = k;
					 }
				 }
			 }
		 }
	 }
	 return result;
 }

#ifndef PARALLEL

//See RSlic2_impl.cpp
 template<typename F>
 RSlic::Voxel::priv::iterateCommonResP iterateCommon(F f, const ClusterSet3 &clusters, int s, ThreadPoolP pool) {
	 auto &&size = clusters.getClusterLabel().size;
	 auto centers = clusters.getCenters();
	 int N = centers.size();
	 return iterateCommonIteration(f, 0, N, size, centers, s, pool);
 }

#else

 //See RSlic2_impl.cpp
 template<typename F>
 RSlic::Voxel::priv::iterateCommonResP iterateCommon(F f, const ClusterSet3 &clusters, int s, ThreadPoolP pool) {
         auto &&size = clusters.getClusterLabel().size;
         auto centers = clusters.getCenters();
         RSlic::Voxel::priv::iterateCommonResP result(new RSlic::Voxel::priv::iterateCommonRes(size));
         int N = centers.size();
         int thread_step = N / pool->threadcount();
         std::vector<std::future<RSlic::Voxel::priv::iterateCommonResP>> futures;
         futures.reserve(pool->threadcount());
         //Map
         for (int thread_start = 0; thread_start < N; thread_start += thread_step) {
                 futures.push_back(pool->enqueue([&](int start) {
                         return iterateCommonIteration(f, start, std::min(start + thread_step, N), size, centers, s, pool);
                 }, thread_start));
         }
         //Reduce
         for (auto &&fut: futures) {
                 auto &&thread_result = fut.get();
                 for (int x = thread_result->calcRect.topFrontLeft.x; x < thread_result->calcRect.bottomBackRight.x; x++) {
                         for (int y = thread_result->calcRect.topFrontLeft.y; y < thread_result->calcRect.bottomBackRight.y; y++) {
                                 for (int t = thread_result->calcRect.topFrontLeft.z; t < thread_result->calcRect.bottomBackRight.z; t++) {
                                         double dist_thread = thread_result->distAt(y, x, t);
                                         double &dist_result = result->distAt(y, x, t);
                                         if (dist_thread < dist_result) {
                                                 dist_result = dist_thread;
                                                 result->labelAt(y, x, t) = thread_result->labelAt(y, x, t);
                                         }
                                 }
                         }
                 }
         }
         return result;
 }

#endif
}

template<typename F>
Slic3P RSlic::Voxel::Slic3::iterate(int stiffness, F f) const {
	const int s = setting->step;

	//Set up the normal Slic version
	RSlic::Voxel::priv::DistNormal<F> distF{setting->img, f, setting->stiffness, s};
	auto res = ::iterateCommon<RSlic::Voxel::priv::DistNormal<F>>(distF, clusters, s, setting->pool);

	//create a new instance
	Settings *newSetting = setting;
	if (setting->stiffness != stiffness) {
		newSetting = new Settings(setting);
		newSetting->stiffness = stiffness;
	}
	Slic3 *result = new Slic3(newSetting, ClusterSet3(res->label, clusters.getCenters().size()), res->dist);
	return shared_ptr<Slic3>(result);
}

namespace {
 template<typename T>
 inline void iterateZeroUpdate3(
		 const MovieCacheP &img, const Mat &label,
		 const vector<Vec3i> &centers, vector<double> &max_dist_color, std::shared_ptr<ThreadPool> pool) {
	 int w = img->width();
	 int h = img->height();
	 int d = img->duration();
	 //Update Slico distance maxima
#ifdef PARALLEL
         std::vector<std::future<void>> results;
         results.reserve(pool->threadcount());
         int step = w / pool->threadcount();
         for (int x = 0; x < w; x += step) {
                 results.push_back(pool->enqueue([&](int xx) {
                         for (int x = xx; x < std::min(xx + step, w); x++) {
#else
	 for (int x = 0; x < w; x++) {
#endif
		 for (int y = 0; y < h; y++) {
			 for (int t = 0; t < d; t++) {
				 RSlic::Voxel::ClusterInt nearest_segment = label.at<RSlic::Voxel::ClusterInt>(y, x, t);
				 if (nearest_segment == -1) continue;
				 auto point = centers[nearest_segment];
				 int py = point[1];
				 int px = point[0];
				 int pt = point[2];
				 auto distColor = RSlic::priv::zero::zeroMetrik(img->at<T>(y, x, t), img->at<T>(py, px, pt));
				 if (max_dist_color.at(nearest_segment) < distColor) {
					 max_dist_color.at(nearest_segment) = distColor;
				 }
			 }
		 }
#ifdef PARALLEL
                         }
                 }, x));
#endif
	 }
#ifdef PARALLEL
         for (auto &res: results) {res.get();}
#endif
 }

 declareCVF_T(iterateZeroUpdate3, iterateZeroUpdate3Helper, return)
}
namespace RSlic {
 namespace Voxel {
  namespace priv {
   //See RSlic2_impl.cpp
   template<typename F>
   struct DistZero {
	   inline double operator()(const Vec3i &point, const Vec3i &center, int clusterIdx) {
		   return f(point, center, img, max_distance[clusterIdx], step);
	   }

	   const RSlic::Voxel::MovieCacheP img;
	   F f;
	   const vector<double> &max_distance;
	   int step;
   };
  }
 }
}

template<typename F>
Slic3P RSlic::Voxel::Slic3::iterateZero(F f) const {
	const int s = setting->step;
	//Set up Slico
	Voxel::priv::DistZero<F> distF{setting->img, f, max_dist_color, s};
	auto res = ::iterateCommon<Voxel::priv::DistZero<F>>(distF, clusters, s, setting->pool);

	//update max_dist_color
	ClusterSet3 newClusters(res->label, clusters.getCenters().size());
	vector<double> new_max_dist_color(max_dist_color);
	::iterateZeroUpdate3Helper(setting->img->type(), setting->img, res->label, newClusters.getCenters(), new_max_dist_color, setting->pool);
	//creating a new instance
	Slic3 *result = new Slic3(setting, std::move(newClusters), res->dist);
	result->max_dist_color = std::move(new_max_dist_color);
	return shared_ptr<Slic3>(result);
}

template<typename F>
Slic3P RSlic::Voxel::Slic3::finalize(F f) const {
	int w = setting->img->width();
	int h = setting->img->height();
	int d = setting->img->duration();
	int *size = setting->img->sizeArray();
	Mat_<ClusterInt> finalClusters(3, size, -1);
	delete size;
	int currentLabel = 0;
	const int lims = setting->step * setting->step * setting->step;// (h * w * d) / (clusters.clusterCount());
	static const int neighboursX[] = {-1, 0, 1, 0, -1, 1, 1, -1, 0, 0};//{1, 0, 0, -1, 0, 0};
	static const int neighboursY[aSize(neighboursX)] = {0, -1, 0, 1, -1, -1, 1, 1, 0, 0};//{0, 1, 0, 0, -1, 0};
	static const int neighboursZ[aSize(neighboursX)] = {0, 0, 0, 0, 0, 0, 0, 0, -1, 1};//{0, 0, 1, 0, 0, -1};

	for (int x = 0; x < w; x++) {
		for (int y = 0; y < h; y++) {
			for (int t = 0; t < d; t++) {
				//Some unassigned pixel?
				if (finalClusters.at<ClusterInt>(y, x, t) == -1) {
					vector<Vec3i> current_points;
					current_points.emplace_back(x, y, t);
					finalClusters.at<ClusterInt>(y, x, t) = currentLabel;

					//Look transitive for all unassigned neighbors
					//set them in current_points and finalCluster
					for (int i = 0; i < current_points.size(); i++) {
						Vec3i point = current_points[i];
						for (int neighbour = 0; neighbour < aSize(neighboursX); neighbour++) {
							int px = point[0] + neighboursX[neighbour];
							int py = point[1] + neighboursY[neighbour];
							int pt = point[2] + neighboursZ[neighbour];
							if (px < 0 || px >= w || py < 0 || py >= h || pt < 0 || pt >= d) continue;
							if (finalClusters.at<ClusterInt>(py, px, pt) == -1
									&& clusters.at(y, x, t) == clusters.at(py, px, pt)) {
								current_points.emplace_back(px, py, pt);
								finalClusters.at<ClusterInt>(py, px, pt) = currentLabel;
							}
						}
					}

					//If there are not enough pixel in the cluster
					//look for the best in the environment and conjoin both
					if (current_points.size() <= lims >> 2) {
						int adjlabel = currentLabel;
						double topdist = DINF;
						
						for (int neighbour = 0; neighbour < aSize(neighboursX); neighbour++) {
							int px = x + neighboursX[neighbour];
							int py = y + neighboursY[neighbour];
							int pt = t + neighboursZ[neighbour];
							if (px < 0 || px >= w || py < 0 || py >= h || pt < 0 || pt >= d) continue;
							ClusterInt label = finalClusters.at<ClusterInt>(py, px, pt);
							if (label >= 0 && label != currentLabel) {
								double dist = f(Vec3i(x, y, t), Vec3i(px, py, pt), setting->img, 1, setting->step);
								if (dist < topdist) { 
									adjlabel = label;
									topdist = dist;
								}
							}
						}
						//Set pixel to this neighbor
						for (const Vec3i point: current_points) { 
							finalClusters.at<ClusterInt>(point[1], point[0], point[2]) = adjlabel;
						}
					} else currentLabel++; 
				}
			}
		}
	}

	Slic3 *result = new Slic3(setting, ClusterSet3(finalClusters, currentLabel), distance);
	return std::shared_ptr<Slic3>(result);
}

#endif // RSlic3_IMPL_H
