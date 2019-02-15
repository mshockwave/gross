#ifndef GROSS_GRAPH_NODEMARKER_H
#define GROSS_GRAPH_NODEMARKER_H
#include "gross/Graph/Node.h"

namespace gross {
// Forward declarations
class Graph;

/// scratch data inside Node that is fast to access
/// note that there should be only one kind of NodeMarker
/// active at a given time
class NodeMarkerBase {
protected:
  using MarkerTy = typename Node::MarkerTy;

private:
  MarkerTy MarkerMin, MarkerMax;

public:
  NodeMarkerBase(Graph& G, unsigned NumState);

  MarkerTy Get(Node* N);

  void Set(Node* N, MarkerTy Val);
};

template<class T>
struct NodeMarker : public NodeMarkerBase {
  NodeMarker(Graph& G, unsigned NumState)
    : NodeMarkerBase(G, NumState) {}

  T Get(Node* N) {
    return static_cast<T>(NodeMarkerBase::Get(N));
  }

  void Set(Node* N, T Val) {
    NodeMarkerBase::Set(N,
                        static_cast<typename NodeMarkerBase::MarkerTy>(Val));
  }
};
} // end namespace gross
#endif
