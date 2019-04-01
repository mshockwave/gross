#include "gross/Graph/AttributeBuilder.h"
#include "gross/Graph/Graph.h"

using namespace gross;

// will clear the Attrs buffer in the current builder
void AttributeBuilder::Attach(Node* N) {
  auto& AttrList = G.Attributes[N];
  AttrList.splice(AttrList.end(), std::move(Attrs));
  AttrSet.clear();
}
