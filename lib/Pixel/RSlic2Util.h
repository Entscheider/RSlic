#ifndef RSlic2UTIL_H
#define RSlic2UTIL_H

#include "RSlic2.h"
#include "RSlic2_impl.h"

/*
 * Helpful functions for Slic and OpenCV
 */
namespace RSlic {
 namespace Pixel {

/**
* Returns the type of the image as a human readable string (e.g. CV_U8C1, CV_16S3, ...)
* @param m the picture
* @return the type as string
*/
  string getType(const Mat &m);


  /**
  * Builds the gradient needed for Slic2 using Sobel.
  * Contains the absolute sum of the derivation of both direction.
  * (If needed mat will be convert into a gray value image)
  * @param mat the picture
  * @return the gradient
  */
  Mat buildGrad(const Mat &mat);

  /**
  * Function that is  described in the paper for computing the metrics (of LAB images) for the algorithm.
  */
  struct distanceColor {

	  inline double operator()(const cv::Vec2i &point, const cv::Vec2i &clusterCenter, const cv::Mat &mat, int stiffness, int step) {
		  if (clusterCenter[1] < 0 || clusterCenter[0] < 0) {
			  return DINF;
		  }
		  cv::Vec3b pixel = mat.at<cv::Vec3b>(point[1], point[0]);
		  cv::Vec3b clust_pixel = mat.at<cv::Vec3b>(clusterCenter[1], clusterCenter[0]);
          int pl = pixel[0], pa = pixel[1], pb = pixel[2];
		  int cl = clust_pixel[0], ca = clust_pixel[1], cb = clust_pixel[2];
		  double dc = pow(pl - cl, 2) + pow(pa - ca, 2) + pow(pb - cb, 2);
		  double ds = pow(point[0] - clusterCenter[0], 2) + pow(point[1] - clusterCenter[1], 2);

		  return dc / stiffness + ds / (step * step);
	  }

  };

  /**
  * Function that is described in the paper for computing the metrics (of gray images) for the algorithm.
  */
  struct distanceGray {
	  inline double operator()(const cv::Vec2i &point, const cv::Vec2i &clusterCenter, const cv::Mat &mat, int stiffness, int step) {
		  if (clusterCenter[1] < 0 || clusterCenter[0] < 0) {
			  return DINF;
		  }
		  int8_t pixel = mat.at<uint8_t>(point[1], point[0]);
		  int8_t clust_pixel = mat.at<uint8_t>(clusterCenter[1], clusterCenter[0]);
		  double dc = abs(pixel - clust_pixel);
		  double ds = sqrt(pow(point[0] - clusterCenter[0], 2) + pow(point[1] - clusterCenter[1], 2));

		  return pow(dc, 2) / stiffness + pow(ds / step, 2);
	  }
  };

  /**
  * Returns Slic2P without any "complicated" parameter.
  * @param m the picture
  * @param the amount of Superpixel (approximately)
  * @param slico use the slico version?
  * @param iterations how many iterations
  * @return instance of Slic2 (shared_ptr) where no iterating or something similar is needed. (error -> nullptr)
  */
  Slic2P shutUpAndTakeMyMoney(const Mat &m, int count = 400, int stiffness = 40, bool slico = false, int iterations = 10);

  /**
  * Heelping for do an iteration by selecting the metrics autmaticly.
  * @param slic the Slic2-Object to iterate
  * @param type the type of the image (img.type() in OpenCV)
  * @param slico using Slico
  * @return the result of slic->iterate or slic->iterateZero with the right metrics.
  * (May nullptr if type is not supported or any other error occurs)
  */
  inline Slic2P iteratingHelper(Slic2P slic, int type, bool slico = false) {
	  if (type == CV_8UC1) {
		  distanceGray g;
		  if (slico) return slic->iterateZero<distanceGray>(g);
		  return slic->iterate<distanceGray>(g);
	  } else if (type == CV_8UC3) {
		  distanceColor c;
		  if (slico) return slic->iterateZero<distanceColor>(c);
		  return slic->iterate<distanceColor>(c);
	  }
	  return Slic2P(); //unsupported type
  }
 }
}
#endif // RSlic2UTIL_H
