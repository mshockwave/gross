#include "gtest/gtest.h"
#include "boost/concept/assert.hpp"
#include "boost/graph/graph_concepts.hpp"
#include "gross/Graph/BGL.h"

TEST(GraphBGLTest, TestGraphConcept) {
  BOOST_CONCEPT_ASSERT(( boost::GraphConcept<gross::Graph> ));
}
