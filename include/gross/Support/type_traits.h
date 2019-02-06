#ifndef GROSS_SUPPORT_TYPE_TRAITS_H
#define GROSS_SUPPORT_TYPE_TRAITS_H
#include <type_traits>

namespace gross {

/// forward porting from C++14
template< bool B, class T = void >
using enable_if_t = typename std::enable_if<B,T>::type;

template<bool B, class VT, VT V>
struct enable_if_v {};

template<class VT, VT V>
struct enable_if_v<true, VT, V> {
  static constexpr VT value = V;
};

template< bool B, class T, class F>
using conditional_t = typename std::conditional<B,T,F>::type;
} // end namespace gross
#endif
