#ifndef GROSS_GRAPH_ATTRIBUTEBUILDER_H
#define GROSS_GRAPH_ATTRIBUTEBUILDER_H
#include "gross/Graph/Attribute.h"
#include <memory>
#include <utility>
#include <list>
#include <set>

namespace gross {
// Forward declaration
class Graph;

struct AttributeBuilder {
  AttributeBuilder(Graph& graph)
    : G(graph) {}

  template<Attr AT, class... CtorArgs>
  AttributeBuilder& Add(CtorArgs &&... args) {
    Attrs.emplace_back(
      new Attribute<AT>(std::forward<CtorArgs>(args)...)
    );
    AttrSet.insert(AT);
    return *this;
  }

  template<Attr AT>
  bool hasAttr() const {
    return AttrSet.count(AT);
  }

  void Attach(Node* N);

  bool empty() const { return Attrs.empty(); }

private:
  Graph& G;
  std::list<std::unique_ptr<AttributeConcept>> Attrs;
  std::set<Attr> AttrSet;
};
} // end namespace gross
#endif
