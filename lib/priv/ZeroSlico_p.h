#ifndef ZEROSLICO_P_H
#define ZEROSLICO_P_H
#include <stdint.h>
#include <type_traits>
namespace RSlic{
namespace priv {
 namespace zero {

  template<typename T>
  inline uint64_t zeroMetrik(T first, T second) {
      static_assert(std::is_arithmetic<T>::value==true,"Need arithmetic type in zeroMetrik");
      return pow(first - second, 2);
  }

  template<typename T, int n>
  inline uint64_t zeroMetrik(const Vec<T, n> &first, const Vec<T, n> &second) {
    static_assert(std::is_arithmetic<T>::value==true,"Need arithmetic type in zeroMetrik");
    uint64_t res = 0;
	  for (int i = 0; i < n; i++) {
		  res += pow(first[i] - second[i], 2);
	  }
	  return res;
  }
 }
}
}
#endif
