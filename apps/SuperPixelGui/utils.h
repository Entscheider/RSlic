#ifndef UTILS_H
#define UTILS_H

#include <opencv2/core/core.hpp>
#include <QtGui/QImage>

namespace utils {
 cv::Mat qImage2Mat(const QImage &img);

 QImage mat2QImage(const cv::Mat &m);

 cv::Mat makeLabIfNecessary(const cv::Mat &m);
}
#endif // UTILS_H
