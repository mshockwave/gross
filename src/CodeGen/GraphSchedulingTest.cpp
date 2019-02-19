#include "boost/concept/assert.hpp"
#include "GraphScheduling.h"
#include "BGL.h"
#include "boost/graph/graph_concepts.hpp"
#include "gtest/gtest.h"

using namespace gross;

TEST(CodeGenUnitTest, TestGraphScheduleConcepts) {
  // requirement for DFS
  BOOST_CONCEPT_ASSERT(( boost::VertexListGraphConcept<gross::GraphSchedule> ));
  BOOST_CONCEPT_ASSERT(( boost::IncidenceGraphConcept<gross::GraphSchedule> ));
  // additional requirement for Graphviz
  BOOST_CONCEPT_ASSERT(( boost::EdgeListGraphConcept<gross::GraphSchedule> ));
  BOOST_CONCEPT_ASSERT(( boost::ReadablePropertyMapConcept<
                          gross::graph_id_map<gross::GraphSchedule,
                                              boost::vertex_index_t>,
                          gross::BasicBlock* > ));
}
