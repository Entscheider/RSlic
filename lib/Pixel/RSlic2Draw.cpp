#include "RSlic2Draw.h"
#include "../priv/Useful.h"

#ifdef DEBUG_ME
#include <iostream>
#endif

#include <type_traits>
#include <tuple>

//To compute mean values there have to be a larger type (e.g. int -> long, vec of int -> vec of long)
//To find this type there are this struct.
//To use: LongVariant<T>::type -> long version of T.
namespace priv {
 //Default
 template<typename T, bool = std::is_integral<T>::value>
 struct LongVariant {
	 using type = T;
 };

 //To distinguish signed form unsigned
 namespace SignedLongTrait {
  template<typename T, bool = std::is_signed<T>::value>
  struct SignedTrait {
  };
  template<typename T>
  struct SignedTrait<T, true> { // signed
	  using type = long;
	  using cvtype = int16_t;
  };
  template<typename T>
  struct SignedTrait<T, false> { // unsigned
	  using type = unsigned long;
	  using cvtype = uint32_t;
  };
 }

 //char, short, int, long
 template<typename T>
 struct LongVariant<T, true> {
	 using type = typename priv::SignedLongTrait::SignedTrait<T>::type;
 };

 namespace VecLongTrait {
  template<template<typename, int> class VT, typename T, int N, bool = std::is_integral<T>::value, bool = std::is_same<VT<T, N>, Vec<T, N>>::value>
  struct VecLongVariant {
  };
  //vec of char, short, int, long
  template<template<typename, int> class VT, typename T, int N>
  struct VecLongVariant<VT, T, N, true, true> {
	  using type = Vec<typename priv::SignedLongTrait::SignedTrait<T>::cvtype, N>;
  };
  //vec of double, float
  template<template<typename, int> class VT, typename T, int N>
  struct VecLongVariant<VT, T, N, false, true> {
	  using type = Vec<double, N>;
  };
 }
 // Vec<..,..>
 template<template<typename, int> class VT, typename T, int N>
 struct LongVariant<VT<T, N>, false> {
	 using type = typename VecLongTrait::VecLongVariant<VT, T, N>::type;
 };

 template<typename tp>
 using LongVector = vector<typename LongVariant<tp>::type>;
}

namespace {
 using namespace RSlic::Pixel;

 template<typename tp>
 inline tuple<priv::LongVector<tp>, vector<int>> calcMean(const Mat &m, const ClusterSet &set) {
	 auto n = set.clusterCount();
	 priv::LongVector<tp> meanValue(n, 0);
	 vector<int> pixelCount(n, 0);
	 auto label = set.getClusterLabel();
	 for (int x = 0; x < m.cols; x++) {
		 for (int y = 0; y < m.rows; y++) {
			 auto idx = label.at<ClusterInt>(y, x);
			 if (idx < 0) continue;
			 meanValue[idx] += m.at<tp>(y, x);
			 pixelCount[idx]++;
		 }
	 }
	 return make_tuple(std::move(meanValue), std::move(pixelCount));
 }


 template<typename tp>
 Mat drawClusterType(const Mat &m, const ClusterSet &set) {
	 Mat res(m.rows, m.cols, m.type());
	 auto label = set.getClusterLabel();
	 auto meantuple = calcMean<tp>(m, set);
	 auto &&meanValue = get<0>(meantuple);
	 auto &&pixelCount = get<1>(meantuple);
	 for (int x = 0; x < m.cols; x++) {
		 for (int y = 0; y < m.rows; y++) {
			 auto idx = label.at<ClusterInt>(y, x);
			 res.at<tp>(y, x) = meanValue[idx] / pixelCount[idx];
		 }
	 }
	 return res;
 }

 declareCVF_T(drawClusterType, drawClusterType_, return cv::Mat())
}

Mat RSlic::Pixel::drawCluster(const Mat &m, const ClusterSet &set) {
	return ::drawClusterType_(m.type(), m, set);
}

