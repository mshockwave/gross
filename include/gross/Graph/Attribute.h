#ifndef GROSS_GRAPH_ATTRIBUTE_H
#define GROSS_GRAPH_ATTRIBUTE_H
#include "gross/Graph/Node.h"

namespace gross {
enum class Attr {
  NoMem,
  Clobber
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
protected:
  Node* AttachedNode;
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

ATTRIBUTE_IMPL(Clobber) {
  Attr Kind() const override { return Attr::Clobber; }
};

#undef ATTRIBUTE_IMPL
} // end namespace gross
#endif
