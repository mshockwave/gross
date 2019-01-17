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

TEST(GraphBGLTest, TestVertexReadablePropertyMapConcept) {
  BOOST_CONCEPT_ASSERT(( boost::ReadablePropertyMapConcept<
                          gross::Graph::id_map<boost::vertex_index_t>,
                          gross::Node* > ));
}
