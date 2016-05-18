#ifndef RSlic3UTILS_H
#define RSlic3UTILS_H

#include "RSlic3.h"

namespace RSlic {
 namespace Voxel {
  /**
  * Simple MovieCache that just load all images into memory.
  */
  class SimpleMovieCache : public MovieCache {
  public:

      /**
      * Initialize with filenames of all images.
      * The images will be converted from BGR to LAB if needed.
      * @param filenames list of files
      */
	  SimpleMovieCache(const std::vector<std::string> &filenames);

	  /**
	  * Initialize with list of all images.
	  * @param pictures list of images
	  */
	  SimpleMovieCache(const std::vector<Mat> &pictures);

      /**
      * Initialize with list of all images. (rvalue)
      * @param pictures list of images
      */
	  SimpleMovieCache(std::vector<Mat> &&pictures);

	  virtual Mat matAt(int t) const override;

	  virtual int duration() const override;

	  /**
	  * Returns list with all images
      * @return list of images
	  */
	  const std::vector<Mat> &getPictures() const;

  private:
	  std::vector<Mat> pictures;
  };

  /**
  * Function for computing the gradient of a gray image.
  * @param img MovieCacheP
  * @param vec the point to compute the gradient
  */
  double buildGradGray(const MovieCacheP &img, const Vec3i &vec);

  /**
  * Function for computing the gradient of a color image.
  * @param img MovieCacheP
  * @param vec the point to compute the gradient
  */
  double buildGradColor(const MovieCacheP &img, const Vec3i &vec);

  /**
  * Function that is  described in the paper for computing the metrics (of LAB images) for the algorithm.
  */
  struct distanceColor{
    inline double operator()(const cv::Vec3i &point, const cv::Vec3i &clusterCenter, const MovieCacheP &mat, int stiffness, int step){
        if (clusterCenter[1] < 0 || clusterCenter[0] < 0 || clusterCenter[2] < 0) {
            return DINF;
        }
        cv::Vec3b pixel = mat->at<cv::Vec3b>(point);
        cv::Vec3b clust_pixel = mat->at<cv::Vec3b>(clusterCenter);
        int pl = pixel[0], pa = pixel[1], pb = pixel[2]; 
        int cl = clust_pixel[0], ca = clust_pixel[1], cb = clust_pixel[2];
        double dc = sqrt(pow(pl - cl, 2) + pow(pb - cb, 2) + pow(pa - ca, 2));
        double ds = sqrt(pow(point[0] - clusterCenter[0], 2) + pow(point[1] - clusterCenter[1], 2) + pow(point[2] - clusterCenter[2], 2));

        return sqrt(pow(dc, 2) / stiffness + pow(ds / step, 2));
    }
  };

  /**
  * Function which is described in the paper for computing the metrics (of gray images) for the algorithm.
  */
  struct distanceGray{
      inline double operator()(const cv::Vec3i &point, const cv::Vec3i &clusterCenter, const MovieCacheP &mat, int stiffness, int step){
          if (clusterCenter[1] < 0 || clusterCenter[0] < 0 || clusterCenter[2] < 0) {
              return DINF;
          }
          int8_t pixel = mat->at<uint8_t>(point);
          int8_t clust_pixel = mat->at<uint8_t>(clusterCenter);
          double dc = abs(pixel - clust_pixel);
          double ds = sqrt(pow(point[0] - clusterCenter[0], 2)
                  + pow(point[1] - clusterCenter[1], 2)
                  + pow(point[2] - clusterCenter[2], 2));
          return /*sqrt(*/pow(dc, 2) / stiffness + pow(ds / step, 2)/*)*/;

      }
  };

#define slicFunHelper(type, fun) \
    type == CV_8UC3? fun(RSlic::Voxel::distanceColor())\
    : (type == CV_8UC1 ? fun(RSlic::Voxel::distanceGray())\
    : nullptr)

  /**
  * Helping for doing an iteration by selecting the metrics automatically.
  * @param slic the Slic2-Object to iterate
  * @param type the type of the image (img.type() in OpenCV)
  * @param slico using Slico
  * @return the result of slic->iterate or slic->iterateZero with the right metrics.
  * (May nullptr if type is not supported or any other error occurs)
  */
  inline Slic3P iterateHelper(Slic3P p, int type, bool slico){
      if (slico){
          return slicFunHelper(type,p->iterateZero);
      }
      return slicFunHelper(type,p->iterate);
  }

  /**
  * Returns Slic3P without any "complicated" parameter.
  * @param m the moviecache
  * @param the amount of Superpixel (approximately)
  * @param slico use the slico version?
  * @param iterations how many iterations
  * @return instance of Slic3 (shared_ptr) where no iterating or something similar is needed. (error -> nullptr)
  */
   Slic3P shutUpAndTakeMyMoney(const RSlic::Voxel::MovieCacheP &m, int count = 4000, int stiffness = 40, bool slico = false, int iterations = 10);
 }
}

#endif
