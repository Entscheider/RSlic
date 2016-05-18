#include "RSlic3.h"
#include <priv/ZeroSlico_p.h>
#include <3rd/ThreadPool.h>
#include <priv/Useful.h>
#include "RSlic3_impl.h"
#include <atomic>

#ifndef u_long
#define u_long unsigned long
#endif
#ifndef uint
#define uint unsigned int
#endif

using RSlic::priv::aSize;
using namespace RSlic::Voxel;


ThreadPoolP Slic3::threadpool() const {
	return setting->pool;
}

Slic3P Slic3::initialize(const MovieCacheP &img, const GradFunc &grad,  int step, int stiffness, ThreadPoolP pool) {
	Slic3::Settings *setting = new Slic3::Settings();
	setting->img = img;
	setting->step = step;
	setting->stiffness = stiffness;
	setting->gradFunc = grad;
	if (pool.get() == nullptr)
		setting->initThread();
	else
		setting->pool = pool;

	Slic3 *res = new Slic3(setting);
	res->init();
	if (res->clusters.clusterCount() == 0) {
		delete res;
		return Slic3P();
	}
	return Slic3P(res);
}

RSlic::Voxel::Slic3::Slic3(Settings *s, ClusterSet3 &&set, const Mat &d) : clusters(std::move(set)), distance(d) {
	assert(s != nullptr);
	setting = s;
	s->__refcount++;
}

RSlic::Voxel::Slic3::~Slic3() {
	int count = --setting->__refcount;
	if (count == 0) {
		delete setting;
	}
}

namespace {
 Vec3i find_local_minimum(const MovieCacheP &p, const GradFunc &f, const Vec3i &center) {
	 auto px = center[0];
	 auto py = center[1];
	 auto pt = center[2];
	 int w = p->width();
	 int h = p->height();
	 int duration = p->duration();
	 double min_value = DINF;
	 cv::Vec3i minPos = center;
	 for (int x = std::max(px - 1, 0); x < std::min(w, px + 2); x++) {
		 for (int y = std::max(0, py - 1); y < std::min(h, py + 2); y++) {
			 for (int t = std::max(0, pt - 1); t < std::min(duration, pt + 2); t++) {
				 Vec3i point(x, y, t);
				 auto currentVal = f(p, point);
				 if (currentVal < min_value) {
					 min_value = currentVal;
					 minPos = point;
				 }
			 }
		 }
	 }
	 return minPos;
 }

}

void RSlic::Voxel::Slic3::init() {
	int s = setting->step;
	int w = setting->img->width();
	int h = setting->img->height();
	int d = setting->img->duration();
	std::vector<Vec3i> centerGrid;
	centerGrid.reserve(w * h * d / s / s / s);
	// initialize the grid and set the minimum of the gradient at once
	for (int x = s; x < w - s / 2; x += s) {
		for (int y = s; y < h - s / 2; y += s) {
			for (int t = s; t < d - s / 2; t += s) {
				Vec3i p(x, y, t);
				centerGrid.push_back(find_local_minimum(setting->img, setting->gradFunc, p)); 
			}
		}
	}


	int *size = setting->img->sizeArray();
	Mat_<ClusterInt> label(3, size, -1);
;
	delete size;
	clusters = ClusterSet3(centerGrid, label);

	max_dist_color = std::vector<double>(clusters.clusterCount(), 1); //for slico
}


int RSlic::Voxel::Slic3::getStep() const {
	return setting->step;
}

int RSlic::Voxel::Slic3::getStiffness() const {
	return setting->stiffness;
}

MovieCacheP RSlic::Voxel::Slic3::getImg() const {
	return setting->img;
}


const ClusterSet3 &RSlic::Voxel::Slic3::getClusters() const {
	return clusters;
}
