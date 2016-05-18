#include "RSlic2Util.h"


string RSlic::Pixel::getType(const Mat &m) {
	switch (m.type()) {
		case CV_8UC1:
			return "CV_8UC1";
		case CV_8UC2:
			return "CV_8UC2";
		case CV_8UC3:
			return "CV_8UC3";
		case CV_8UC4:
			return "CV_8UC4";

		case CV_8SC1:
			return "CV_8SC1";
		case CV_8SC2:
			return "CV_8SC2";
		case CV_8SC3:
			return "CV_8SC3";
		case CV_8SC4:
			return "CV_8SC4";

		case CV_16UC1:
			return "CV_16UC1";
		case CV_16UC2:
			return "CV_16UC2";
		case CV_16UC3:
			return "CV_16UC3";
		case CV_16UC4:
			return "CV_16WUC4";

		case CV_16SC1:
			return "CV_16SC1";
		case CV_16SC2:
			return "CV_16SC2";
		case CV_16SC3:
			return "CV_16SC3";
		case CV_16SC4:
			return "CV_16SC4";

		case CV_32SC1:
			return "CV_32SC1";
		case CV_32SC2:
			return "CV_32SC2";
		case CV_32SC3:
			return "CV_32SC3";
		case CV_32SC4:
			return "CV_32SC4";

		case CV_32FC1:
			return "CV_32FC1";
		case CV_32FC2:
			return "CV_32FC2";
		case CV_32FC3:
			return "CV_32FC3";
		case CV_32FC4:
			return "CV_32FC4";

		case CV_64FC1:
			return "CV_64FC1";
		case CV_64FC2:
			return "CV_64FC2";
		case CV_64FC3:
			return "CV_64FC3";
		case CV_64FC4:
			return "CV_64FC4";

		default:
			return "Unknown";
	}
}


Mat RSlic::Pixel::buildGrad(const Mat &mat) {
	cv::Mat dx, dy, res;
	cv::Mat mat_gray;
	switch (mat.type()) {
		case CV_8UC3:
			cv::cvtColor(mat, mat_gray, cv::COLOR_BGR2GRAY);
			break;
		case CV_8UC4:
			cv::cvtColor(mat, mat_gray, cv::COLOR_BGRA2GRAY);
			break;
		default:
			mat_gray = mat;
	}
	mat_gray.convertTo(mat_gray, CV_32FC1);
	cv::Sobel(mat_gray, dx, -1, 1, 0);
	cv::pow(dx, 2, dx);
	cv::Sobel(mat_gray, dy, -1, 0, 1);
	cv::pow(dy, 2, dy);
	cv::sqrt(dx + dy, res);
	return res;
}

template <typename F>
static RSlic::Pixel::Slic2P shutUpAndTakeMyMoneyType(const Mat &m, int step, int stiffness, bool slico, int iterations) {
    F f;
    Mat grad = RSlic::Pixel::buildGrad(m);
    auto res = RSlic::Pixel::Slic2::initialize(m,grad,step, stiffness);
    if (res.get() == nullptr) return res; //error
    for (int i=0; i< iterations; i++){
        if (slico) res = res->iterateZero<F>(f);
        else res = res->iterate<F>(f);
    }
    res = res->finalize<F>(f);
    return res;
}


RSlic::Pixel::Slic2P  RSlic::Pixel::shutUpAndTakeMyMoney(const Mat &m, int count, int stiffness, bool slico, int iterations) {
	int w = m.cols;
	int h = m.rows;
	int step = sqrt(w * h * 1.0 / count);

    if (m.type() == CV_8UC3) {
        return shutUpAndTakeMyMoneyType<distanceColor>(m,step, stiffness,slico,iterations);
    }
    if (m.type() == CV_8UC4) {
		Mat other;
		cv::cvtColor(m, other, cv::COLOR_BGRA2BGR);
        return shutUpAndTakeMyMoneyType<distanceColor>(other,step, stiffness,slico,iterations);
	}//TODO: More Types
	if (m.type() == CV_8UC1) {
    	return shutUpAndTakeMyMoneyType<distanceGray>(m,step, stiffness,slico,iterations);
	}
	return nullptr;
}
