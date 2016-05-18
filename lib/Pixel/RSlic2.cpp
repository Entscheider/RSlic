#include "RSlic2.h"

#include <3rd/ThreadPool.h>
#include <priv/Useful.h>
#include <array>
#include <atomic>
#include <limits.h>

#include "RSlic2_impl.h"

#ifndef u_long
#define u_long unsigned long
#endif
#ifndef uint
#define uint unsigned int
#endif

using RSlic::priv::aSize;
using namespace RSlic;

Slic2P RSlic::Pixel::Slic2::initialize(const Mat &img, const Mat &grad, int step, int stiffness, ThreadPoolP pool) {
	Slic2::Settings *setting = new Slic2::Settings();
	setting->img = img;
	setting->step = step;
	setting->stiffness = stiffness;

	if (pool.get() == nullptr)
		setting->initThreadPool();
	else
		setting->pool = pool;

	Slic2 *res = new Slic2(setting);
	res->init(grad);
	if (res->getClusters().clusterCount() == 0)
		return Slic2P();
	return Slic2P(res);
}

ThreadPoolP RSlic::Pixel::Slic2::threadpool() const {
	return setting->pool;
}

RSlic::Pixel::Slic2::Slic2(Slic2::Settings *s, ClusterSet &&c, const Mat &d) : clusters(std::move(c)), distance(d) {
	assert(s != nullptr);
	setting = s;
	s->__refcount++;
}

RSlic::Pixel::Slic2::~Slic2() {
	int count = --setting->__refcount;
	if (count == 0) {
		delete setting;
	}
}

namespace {

 template<typename gtp>
 Vec2i find_local_minimum(const Mat &grad, const Vec2i &center) {
	 auto px = center[0];
	 auto py = center[1];
	 gtp min_value = std::numeric_limits<gtp>::infinity();
	 cv::Vec2i minPos = center;
	 for (int x = std::max(px - 1, 0); x < std::min(grad.cols, px + 2); x++) {
		 for (int y = std::max(0, py - 1); y < std::min(grad.rows, py + 2); y++) {
			 const gtp currentVal = grad.at<gtp>(y, x);
			 if (currentVal < min_value) {
				 min_value = currentVal;
				 minPos = Vec2i(x, y);
			 }
		 }
	 }
	 return minPos;
 }

 //find_local_minimum needs a type as template argument.
 //find_local_minimum_helper will call find_local_minimum with the right template.
 declareCVF_D(find_local_minimum, find_local_minimum_helper, return Vec2i(-1, -1))

 /**
 * Finds the local minimum in a 3x3-neighbourhood.
 * @param grad the gradient of the image
 * @param p the point
 * @return the point with the lowest gradient value.
 */
 Vec2i find_local_minimum_(const cv::Mat &grad, const Vec2i &p) {
	 auto res = ::find_local_minimum_helper(grad.type(), grad, p);
	 if (res == Vec2i(-1, -1)) return p;
	 return res;
 }
}

void RSlic::Pixel::Slic2::init(const Mat &grad) {
	int s = setting->step;
	std::vector<Vec2i> centerGrid;
	centerGrid.reserve(setting->img.cols / s * setting->img.rows / s);
	// initialise the grid and set the its points to the lowest gradient at once
	for (int x = s; x < setting->img.cols - s / 2; x += s) {
		for (int y = s; y < setting->img.rows - s / 2; y += s) {
			Vec2i p(x, y);
			centerGrid.push_back(find_local_minimum_(grad, p));
		}
	}


	Mat_<ClusterInt> label(setting->img.rows, setting->img.cols, -1);

	distance = Mat_<double>(setting->img.rows, setting->img.cols, DINF);

	clusters = ClusterSet(centerGrid, label);

	max_dist_color = vector<double>(centerGrid.size(), 1); //for slico
}

int RSlic::Pixel::Slic2::getStep() const {
	return setting->step;
}

int RSlic::Pixel::Slic2::getStiffness() const {
	return setting->stiffness;
}

Mat RSlic::Pixel::Slic2::getImg() const {
	return setting->img;
}


const RSlic::Pixel::ClusterSet &RSlic::Pixel::Slic2::getClusters() const {
	return clusters;
}
