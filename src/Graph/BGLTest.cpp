#include "gtest/gtest.h"
#include "boost/concept/assert.hpp"
#include "gross/Graph/Graph.h"
#include "gross/Graph/BGL.h"
#include "boost/graph/graph_concepts.hpp"

TEST(GraphBGLTest, TestGraphConcept) {
  BOOST_CONCEPT_ASSERT(( boost::GraphConcept<gross::Graph> ));
}
TEST(GraphBGLTest, TestVertexListGraphConcept) {
  BOOST_CONCEPT_ASSERT(( boost::VertexListGraphConcept<gross::Graph> ));
}
TEST(GraphBGLTest, TestEdgeListGraphConcept) {
  BOOST_CONCEPT_ASSERT(( boost::EdgeListGraphConcept<gross::Graph> ));
}
TEST(GraphBGLTest, TestIncidenceGraphConcept) {
  BOOST_CONCEPT_ASSERT(( boost::IncidenceGraphConcept<gross::Graph> ));
}

TEST(GraphBGLTest, TestVertexReadablePropertyMapConcept) {
  BOOST_CONCEPT_ASSERT(( boost::ReadablePropertyMapConcept<
                          gross::graph_id_map<gross::Graph,
                                              boost::vertex_index_t>,
                          gross::Node* > ));
}

TEST(SubGraphBGLTest, TestGraphConcept) {
  BOOST_CONCEPT_ASSERT(( boost::GraphConcept<gross::SubGraph> ));
}
TEST(SubGraphBGLTest, TestVertexListGraphConcept) {
  BOOST_CONCEPT_ASSERT(( boost::VertexListGraphConcept<gross::SubGraph> ));
}
TEST(SubGraphBGLTest, TestEdgeListGraphConcept) {
  BOOST_CONCEPT_ASSERT(( boost::EdgeListGraphConcept<gross::SubGraph> ));
}
TEST(SubGraphBGLTest, TestIncidenceGraphConcept) {
  BOOST_CONCEPT_ASSERT(( boost::IncidenceGraphConcept<gross::SubGraph> ));
}

TEST(SubGraphBGLTest, TestVertexReadablePropertyMapConcept) {
  BOOST_CONCEPT_ASSERT(( boost::ReadablePropertyMapConcept<
                          gross::graph_id_map<gross::SubGraph,
                                              boost::vertex_index_t>,
                          gross::Node* > ));
}
