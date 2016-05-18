#ifndef USEFUL_H
#define USEFUL_H
#include <opencv2/core/core.hpp>
namespace RSlic{
namespace priv{

 /**
  * Returns the size of an array (compile-time)
  */
 template<typename T, std::size_t N>
 constexpr std::size_t aSize(T(&)[N]) noexcept { return N;}


 template <int n>
 struct cvtp{using type= int;}; //Default
 template <>
 struct cvtp<CV_8SC1>{using type= int8_t;};
 template <>
 struct cvtp<CV_8SC2>{using type= cv::Vec<int8_t,2>;};
 template <>
 struct cvtp<CV_8SC3>{using type= cv::Vec<int8_t,3>;};
 template <>
 struct cvtp<CV_8SC4>{using type= cv::Vec<int8_t,4>;};
 template <>
 struct cvtp<CV_8UC1>{using type= uint8_t;};
 template <>
 struct cvtp<CV_8UC2>{using type= cv::Vec2b;};
 template <>
 struct cvtp<CV_8UC3>{using type= cv::Vec3b;};
 template <>
 struct cvtp<CV_8UC4>{using type= cv::Vec4b;};
 template <>
 struct cvtp<CV_16UC1>{using type=uint16_t;};
 template <>
 struct cvtp<CV_16UC2>{using type=cv::Vec2w;};
 template <>
 struct cvtp<CV_16UC3>{using type=cv::Vec3w;};
 template <>
 struct cvtp<CV_16UC4>{using type=cv::Vec4w;};
 template <>
 struct cvtp<CV_16SC1>{using type=int16_t;};
 template <>
 struct cvtp<CV_16SC2>{using type=cv::Vec2s;};
 template <>
 struct cvtp<CV_16SC3>{using type=cv::Vec3s;};
 template <>
 struct cvtp<CV_16SC4>{using type=cv::Vec4s;};
 template <>
 struct cvtp<CV_32SC1>{using type=int32_t;};
 template <>
 struct cvtp<CV_32SC2>{using type=cv::Vec<int32_t,2>;};
 template <>
 struct cvtp<CV_32SC3>{using type=cv::Vec<int32_t,3>;};
 template <>
 struct cvtp<CV_32SC4>{using type=cv::Vec<int32_t,4>;};
 template <>
 struct cvtp<CV_32FC1>{using type=float;};
 template <>
 struct cvtp<CV_32FC2>{using type=cv::Vec2f;};
 template <>
 struct cvtp<CV_32FC3>{using type=cv::Vec3f;};
 template <>
 struct cvtp<CV_32FC4>{using type=cv::Vec4f;};
 template <>
 struct cvtp<CV_64FC1>{using type=double;};
 template <>
 struct cvtp<CV_64FC2>{using type=cv::Vec2d;};
 template <>
 struct cvtp<CV_64FC3>{using type=cv::Vec3d;};
 template <>
 struct cvtp<CV_64FC4>{using type=cv::Vec4d;};
}
}

// Creates function which calls f with the right template argument based on the type 
// f -> template Function, ff -> name of new function, def -> comes after default in switch(type){....; default: def}
#define declareCVF_T(f,ff,def)\
    template <typename... T>\
    inline auto ff(int type, T&&... params) -> typename std::result_of<decltype(f<uint8_t>)&(T...)>::type{\
    using namespace RSlic::priv;\
    static_assert(sizeof(float) * CHAR_BIT == 32,"Float Size != 32 Bit");\
    static_assert(sizeof(double) * CHAR_BIT == 64,"Double Size != 64 Bit");\
    switch(type){\
    case CV_8UC1: return f<cvtp<CV_8UC1>::type>(std::forward<T>(params)...);\
    case CV_8UC2: return f<cvtp<CV_8UC2>::type>(std::forward<T>(params)...);\
    case CV_8UC3: return f<cvtp<CV_8UC3>::type>(std::forward<T>(params)...);\
    case CV_8UC4: return f<cvtp<CV_8UC4>::type>(std::forward<T>(params)...);\
    case CV_8SC1: return f<cvtp<CV_8SC1>::type>(std::forward<T>(params)...);\
    case CV_8SC2: return f<cvtp<CV_8SC2>::type>(std::forward<T>(params)...);\
    case CV_8SC3: return f<cvtp<CV_8SC3>::type>(std::forward<T>(params)...);\
    case CV_8SC4: return f<cvtp<CV_8SC4>::type>(std::forward<T>(params)...);\
    case CV_16UC1: return f<cvtp<CV_16UC1>::type>(std::forward<T>(params)...);\
    case CV_16UC2: return f<cvtp<CV_16UC2>::type>(std::forward<T>(params)...);\
    case CV_16UC3: return f<cvtp<CV_16UC3>::type>(std::forward<T>(params)...);\
    case CV_16UC4: return f<cvtp<CV_16UC4>::type>(std::forward<T>(params)...);\
    case CV_16SC1: return f<cvtp<CV_16SC1>::type>(std::forward<T>(params)...);\
    case CV_16SC2: return f<cvtp<CV_16SC2>::type>(std::forward<T>(params)...);\
    case CV_16SC3: return f<cvtp<CV_16SC3>::type>(std::forward<T>(params)...);\
    case CV_16SC4: return f<cvtp<CV_16SC4>::type>(std::forward<T>(params)...);\
    case CV_32SC1: return f<cvtp<CV_32SC1>::type>(std::forward<T>(params)...);\
    case CV_32SC2: return f<cvtp<CV_32SC2>::type>(std::forward<T>(params)...);\
    case CV_32SC3: return f<cvtp<CV_32SC3>::type>(std::forward<T>(params)...);\
    case CV_32SC4: return f<cvtp<CV_32SC4>::type>(std::forward<T>(params)...);\
    case CV_32FC1: return f<cvtp<CV_32FC1>::type>(std::forward<T>(params)...);\
    case CV_32FC2: return f<cvtp<CV_32FC2>::type>(std::forward<T>(params)...);\
    case CV_32FC3: return f<cvtp<CV_32FC3>::type>(std::forward<T>(params)...);\
    case CV_32FC4: return f<cvtp<CV_32FC4>::type>(std::forward<T>(params)...);\
    case CV_64FC1: return f<cvtp<CV_64FC1>::type>(std::forward<T>(params)...);\
    case CV_64FC2: return f<cvtp<CV_64FC2>::type>(std::forward<T>(params)...);\
    case CV_64FC3: return f<cvtp<CV_64FC3>::type>(std::forward<T>(params)...);\
    case CV_64FC4: return f<cvtp<CV_64FC4>::type>(std::forward<T>(params)...);\
    default: def;\
    }}

// Same as declareCVF_T, but does not use the depth.
#define declareCVF_D(f,ff,d)\
    template <typename... T>\
    inline auto ff(int type, T&&... params) -> typename std::result_of<decltype(f<uint8_t>)&(T...)>::type{\
    using namespace RSlic::priv;\
    static_assert(sizeof(float) * CHAR_BIT == 32,"Float Size != 32 Bit");\
    static_assert(sizeof(double) * CHAR_BIT == 64,"Double Size != 64 Bit");\
    switch(type){\
    case CV_8U: return f<cvtp<CV_8U>::type>(std::forward<T>(params)...);\
    case CV_8S: return f<cvtp<CV_8S>::type>(std::forward<T>(params)...);\
    case CV_16U: return f<cvtp<CV_16U>::type>(std::forward<T>(params)...);\
    case CV_16S: return f<cvtp<CV_16S>::type>(std::forward<T>(params)...);\
    case CV_32S: return f<cvtp<CV_32S>::type>(std::forward<T>(params)...);\
    case CV_32F: return f<cvtp<CV_32F>::type>(std::forward<T>(params)...);\
    case CV_64F: return f<cvtp<CV_64F>::type>(std::forward<T>(params)...);\
    default: d;\
    }}

#endif // USEFUL_H
