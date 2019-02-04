#ifndef GROSS_SUPPORT_STLEXTRAS_H
#define GROSS_SUPPORT_STLEXTRAS_H
#include <algorithm>
#include <utility>
#include <iterator>

namespace gross {
// traits to unwrap std::unique_ptr to underlying pointer
template<class T>
struct unique_ptr_unwrapper {
  T* operator()(std::unique_ptr<T>& UP) const {
    return UP.get();
  }
};

template <typename ContainerTy>
auto adl_begin(ContainerTy &&container)
    -> decltype(std::begin(std::forward<ContainerTy>(container))) {
  return std::begin(std::forward<ContainerTy>(container));
}

template <typename ContainerTy>
auto adl_end(ContainerTy &&container)
    -> decltype(std::end(std::forward<ContainerTy>(container))) {
  return std::end(std::forward<ContainerTy>(container));
}

/// Provide wrappers to std::find which take ranges instead of having to pass
/// begin/end explicitly.
template <typename R, typename T>
auto find(R &&Range, const T &Val) -> decltype(adl_begin(Range)) {
  return std::find(adl_begin(Range), adl_end(Range), Val);
}

/// Provide wrappers to std::find_if which take ranges instead of having to pass
/// begin/end explicitly.
template <typename R, typename UnaryPredicate>
auto find_if(R &&Range, UnaryPredicate P) -> decltype(adl_begin(Range)) {
  return std::find_if(adl_begin(Range), adl_end(Range), P);
}

template <typename R, typename UnaryPredicate>
auto find_if_not(R &&Range, UnaryPredicate P) -> decltype(adl_begin(Range)) {
  return std::find_if_not(adl_begin(Range), adl_end(Range), P);
}
} // end namespace gross
#endif
