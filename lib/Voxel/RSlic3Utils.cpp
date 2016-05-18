#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include "RSlic3Utils.h"
#include "RSlic3_impl.h"

cv::Mat RSlic::Voxel::SimpleMovieCache::matAt(int t) const {
  if (t >= pictures.size()) return Mat();
  return pictures[t];
}

int RSlic::Voxel::SimpleMovieCache::duration() const {
  return pictures.size();
}

RSlic::Voxel::SimpleMovieCache::SimpleMovieCache(const std::vector<Mat> &_pictures) : pictures(_pictures) {

}


RSlic::Voxel::SimpleMovieCache::SimpleMovieCache(std::vector<Mat> &&_pictures) : pictures(std::move(_pictures)) {

}

RSlic::Voxel::SimpleMovieCache::SimpleMovieCache(const std::vector<std::string> &filenames) {
  for (const string &f: filenames) {
    Mat mat = cv::imread(f, cv::IMREAD_COLOR); //TODO: Don't ignore cases where images have not the same size or type 
    switch (mat.type()) {
      case CV_8UC3:
        cv::cvtColor(mat, mat, cv::COLOR_BGR2Lab);
        break;
    }
    pictures.push_back(mat);
  }
}

namespace {
  template<typename T>
    inline double decolor(const T &color) {
      return (double) color;
    }

  template<>
    inline double decolor<Vec3b>(const Vec3b &color) {
      return (color[0] * 1.0 + color[1] * 1.0 + color[2] * 1.0 ) / 3.0;
    }

  template<>
    inline double decolor<uint8_t>(const uint8_t &color) {
      return (double) color;
    }

  template<typename T>
    inline double buildGradIntern(RSlic::Voxel::MovieCacheP const &img, Vec3i const &vec) {
      int y = vec[1];
      int x = vec[0];
      int t = vec[2];

      double dx = decolor(img->at<T>(y, x - 1, t) + img->at<T>(y, x + 1, t));
      dx = dx / 2;

      double dy = decolor(img->at<T>(y - 1, x, t) + img->at<T>(y + 1, x, t));
      dy = dy / 2;

      double dt = decolor(img->at<T>(y, x, t - 1) + img->at<T>(y, x, t + 1));
      dt = dt / 2;

      return sqrt(dx * dx + dy * dy + dt * dt);
    }
}


double ::RSlic::Voxel::buildGradGray(RSlic::Voxel::MovieCacheP const &img, Vec3i const &vec) {
  return ::buildGradIntern<uint8_t>(img, vec);
}

double ::RSlic::Voxel::buildGradColor(RSlic::Voxel::MovieCacheP const &img, const Vec3i &vec) {
  return ::buildGradIntern<Vec3b>(img, vec);
}

template <typename F>
static RSlic::Voxel::Slic3P shutUpAndTakeMyMoneyType(const RSlic::Voxel::MovieCacheP &m, int step, int stiffness, bool slico, int iterations,  function<double(const RSlic::Voxel::MovieCacheP &, const Vec3i &)> grad) {
  F f;
  auto res = RSlic::Voxel::Slic3::initialize(m,grad,step, stiffness);
  if (res.get() == nullptr) return res; //error
  for (int i=0; i< iterations; i++){
    if (slico) res = res->iterateZero<F>(f);
    else res = res->iterate<F>(f);
  }
  res = res->finalize<F>(f);
  return res;
}


RSlic::Voxel::Slic3P  RSlic::Voxel::shutUpAndTakeMyMoney(const RSlic::Voxel::MovieCacheP &m, int count, int stiffness, bool slico, int iterations) {
  int w = m->width();
  int h = m->height();
  int t = m->duration();
  int step = pow(w * h * t * 1.0 / count, 1.0/3);

  if (m->type() == CV_8UC3) {
    return shutUpAndTakeMyMoneyType<distanceColor>(m,step, stiffness,slico,iterations, buildGradColor);
  }
  else if (m->type() == CV_8UC1){
    return shutUpAndTakeMyMoneyType<distanceGray>(m,step, stiffness, slico,iterations, buildGradGray);
  }
  return nullptr;
}
