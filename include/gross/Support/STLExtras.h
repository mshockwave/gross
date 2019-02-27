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

/// Constructs a `new T()` with the given args and returns a
///        `unique_ptr<T>` which owns the object.
///
/// Example:
///
///     auto p = make_unique<int>();
///     auto p = make_unique<std::tuple<int, int>>(0, 1);
template <class T, class... Args>
typename std::enable_if<!std::is_array<T>::value, std::unique_ptr<T>>::type
make_unique(Args &&... args) {
  return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
}

/// Constructs a `new T[n]` with the given args and returns a
///        `unique_ptr<T[]>` which owns the object.
///
/// \param n size of the new array.
///
/// Example:
///
///     auto p = make_unique<int[]>(2); // value-initializes the array with 0's.
template <class T>
typename std::enable_if<std::is_array<T>::value && std::extent<T>::value == 0,
                        std::unique_ptr<T>>::type
make_unique(size_t n) {
  return std::unique_ptr<T>(new typename std::remove_extent<T>::type[n]());
}

/// This function isn't used and is only here to provide better compile errors.
template <class T, class... Args>
typename std::enable_if<std::extent<T>::value != 0>::type
make_unique(Args &&...) = delete;
} // end namespace gross
#endif
