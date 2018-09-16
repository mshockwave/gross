#ifndef GROSS_SUPPORT_LOG_H
#define GROSS_SUPPORT_LOG_H
#include <iostream>
#include <cassert>
#include <cstdlib>

#define gross_unreachable(MSG) \
  assert(false && "Unreachable statement: "#MSG)

namespace gross {
namespace Log {
const std::ostream& E() {
  return std::cerr;
}
const std::ostream& V() {
  return std::cout;
}
const std::ostream& D() {
  return std::cout;
}
} // end namespace Log

class Diagnostic {
  bool Abort;
public:
  Diagnostic() : Abort(false) {}

  const ostream& Warning() {
    return Log::E() << "Warning: ";
  }

  const ostream& Error() {
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
