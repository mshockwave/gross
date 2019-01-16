#ifndef GROSS_SUPPORT_STLEXTRAS_H
#define GROSS_SUPPORT_STLEXTRAS_H

namespace gross {
// traits to unwrap std::unique_ptr to underlying pointer
template<class T>
struct unique_ptr_unwrapper {
  T* operator()(std::unique_ptr<T>& UP) const {
    return UP.get();
  }
};
} // end namespace gross
#endif
