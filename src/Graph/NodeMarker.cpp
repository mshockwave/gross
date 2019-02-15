#include "gross/Graph/Graph.h"
#include "gross/Graph/NodeMarker.h"

using namespace gross;

NodeMarkerBase::NodeMarkerBase(Graph& G, unsigned NumState)
  : MarkerMin(G.MarkerMax), MarkerMax(G.MarkerMax += NumState){
  assert(NumState != 0U);
  assert(MarkerMin < MarkerMax && "Wraparound!");
}

NodeMarkerBase::MarkerTy
NodeMarkerBase::Get(Node* N) {
  auto Data = N->MarkerData;
  if(Data < MarkerMin) return 0;
  assert(Data < MarkerMax &&
         "Using an old NodeMarker?");
  return Data - MarkerMin;
}

void NodeMarkerBase::Set(Node* N, NodeMarkerBase::MarkerTy NewMarker) {
  assert(NewMarker < (MarkerMax - MarkerMin));
  assert(N->MarkerData < MarkerMax);
  N->MarkerData = (MarkerMin + NewMarker);
}
