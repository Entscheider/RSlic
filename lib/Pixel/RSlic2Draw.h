#ifndef RSlic2DRAW_H
#define RSlic2DRAW_H

#include "RSlic2.h"
/*
 * Functions for drawing cluster
 */

namespace RSlic {
 namespace Pixel {
  /**
  * Colorize the cluster by using the mean color.
  * @param m the picture
  * @param set the ClusterSet
  * @return new picutre with colorized cluster
  */
  Mat drawCluster(const Mat &m, const ClusterSet &set);

  /**
  * Draw the lines around the cluster. color_t have to be right type for the image.
  * @param m the picture
  * @param set the ClusterSet
  * @param color the color for drawing
  * @return a new image
  */
  template<typename color_t>
  Mat contourCluster(const Mat &m, const ClusterSet &set, color_t color) {
	  Mat res = m.clone();
	  for (int x = 1; x < m.cols - 1; x++) {
		  for (int y = 1; y < m.rows - 1; y++) {
			  int currentLabel = set.at(y, x);
			  if (set.at(y + 1, x) != currentLabel
					  || set.at(y - 1, x) != currentLabel
					  || set.at(y, x + 1) != currentLabel
					  || set.at(y, x - 1) != currentLabel) {
				  res.at<color_t>(y, x) = color;
			  }
		  }
	  }
	  return res;
  }
 }
}

#endif // RSlic2DRAW_H
