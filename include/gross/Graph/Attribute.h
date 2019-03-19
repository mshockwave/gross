#ifndef GROSS_GRAPH_ATTRIBUTE_H
#define GROSS_GRAPH_ATTRIBUTE_H
#include "gross/Graph/Node.h"

namespace gross {
enum class Attr {
  NoMem,
  ReadMem,
  WriteMem,
  HasSideEffect, // enviornment side-effects
  IsBuiltin
};

// Forward declaration
template<Attr AT>
class Attribute;

struct AttributeConcept {
  template<Attr AT>
  Attribute<AT>& as() {
    assert(Kind() == AT && "Invalid Attr kind");
    return *static_cast<Attribute<AT>*>(this);
  }
  template<Attr AT>
  const Attribute<AT>& as() const {
    assert(Kind() == AT && "Invalid Attr kind");
    return *static_cast<const Attribute<AT>*>(this);
  }

  virtual Attr Kind() const = 0;

  virtual ~AttributeConcept() {}
};

// default implementation
template<Attr AT>
class Attribute : public AttributeConcept {
  Attr Kind() const override {
    gross_unreachable("Unimplemented");
  }
};

#define ATTRIBUTE_IMPL(AT) \
  template<>  \
  class Attribute<Attr::AT> : public AttributeConcept

ATTRIBUTE_IMPL(NoMem) {
  Attr Kind() const override { return Attr::NoMem; }
};

// Both Read/WriteMem implies some really coarse grain
// global memory access mode. It won't tell which
// global memory does it access.
ATTRIBUTE_IMPL(ReadMem) {
  Attr Kind() const override { return Attr::ReadMem; }
};
ATTRIBUTE_IMPL(WriteMem) {
  Attr Kind() const override { return Attr::WriteMem; }
};

// Environment side-effects (e.g. read/write output)
ATTRIBUTE_IMPL(HasSideEffect) {
  Attr Kind() const override { return Attr::HasSideEffect; }
};
// Is builtin functions
ATTRIBUTE_IMPL(IsBuiltin) {
  Attr Kind() const override { return Attr::IsBuiltin; }
};
#undef ATTRIBUTE_IMPL
} // end namespace gross
#endif
