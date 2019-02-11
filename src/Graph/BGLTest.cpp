#include "gtest/gtest.h"
#include "boost/concept/assert.hpp"
#include "gross/Graph/Graph.h"
#include "gross/Graph/BGL.h"
#include "boost/graph/graph_concepts.hpp"

TEST(GraphBGLUnitTest, TestGraphConcept) {
  BOOST_CONCEPT_ASSERT(( boost::GraphConcept<gross::Graph> ));
}
TEST(GraphBGLUnitTest, TestVertexListGraphConcept) {
  BOOST_CONCEPT_ASSERT(( boost::VertexListGraphConcept<gross::Graph> ));
}
TEST(GraphBGLUnitTest, TestEdgeListGraphConcept) {
  BOOST_CONCEPT_ASSERT(( boost::EdgeListGraphConcept<gross::Graph> ));
}
TEST(GraphBGLUnitTest, TestIncidenceGraphConcept) {
  BOOST_CONCEPT_ASSERT(( boost::IncidenceGraphConcept<gross::Graph> ));
}

TEST(GraphBGLUnitTest, TestVertexReadablePropertyMapConcept) {
  BOOST_CONCEPT_ASSERT(( boost::ReadablePropertyMapConcept<
                          gross::graph_id_map<gross::Graph,
                                              boost::vertex_index_t>,
                          gross::Node* > ));
}

TEST(SubGraphBGLUnitTest, TestGraphConcept) {
  BOOST_CONCEPT_ASSERT(( boost::GraphConcept<gross::SubGraph> ));
}
TEST(SubGraphBGLUnitTest, TestVertexListGraphConcept) {
  BOOST_CONCEPT_ASSERT(( boost::VertexListGraphConcept<gross::SubGraph> ));
}
TEST(SubGraphBGLUnitTest, TestEdgeListGraphConcept) {
  BOOST_CONCEPT_ASSERT(( boost::EdgeListGraphConcept<gross::SubGraph> ));
}
TEST(SubGraphBGLUnitTest, TestIncidenceGraphConcept) {
  BOOST_CONCEPT_ASSERT(( boost::IncidenceGraphConcept<gross::SubGraph> ));
}

TEST(SubGraphBGLUnitTest, TestVertexReadablePropertyMapConcept) {
  BOOST_CONCEPT_ASSERT(( boost::ReadablePropertyMapConcept<
                          gross::graph_id_map<gross::SubGraph,
                                              boost::vertex_index_t>,
                          gross::Node* > ));
}
