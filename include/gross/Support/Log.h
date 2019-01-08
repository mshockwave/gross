#ifndef GROSS_SUPPORT_LOG_H
#define GROSS_SUPPORT_LOG_H
#include <iostream>
#include <cassert>
#include <cstdlib>

#define gross_unreachable(MSG) \
  assert(false && "Unreachable statement: "#MSG)

namespace gross {
namespace Log {
inline
std::ostream& E() {
  return std::cerr << "[Error] ";
}

inline
std::ostream& V() {
  return std::cout << "[Verbose] ";
}

inline
std::ostream& D() {
  return std::cout << "[Debug] ";
}
} // end namespace Log

class Diagnostic {
  bool Abort;
public:
  Diagnostic() : Abort(false) {}

  std::ostream& Warning() {
    return Log::E() << "Warning: ";
  }

  std::ostream& Error() {
    Abort = true;
    return Log::E() << "Error: ";
  }

  ~Diagnostic() {
    if(Abort)
      std::exit(1);
  }
};
} // end namespace gross
#endif
