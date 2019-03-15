#include "Parser.h"
#include "gross/Graph/Graph.h"
#include "gtest/gtest.h"
#include <string>
#include <sstream>
#include <fstream>

using namespace gross;

TEST(ParserUtilsUnitTest, TestAffineRecordTable) {
  AffineRecordTable<std::string,int> ART;
  using affine_table_type = decltype(ART);
  using table_type = typename affine_table_type::TableTy;
  // should create a default scope and table
  EXPECT_EQ(ART.num_scopes(), 1);
  EXPECT_EQ(ART.num_tables(), 1);
  ART["hello"] = 8;
  ART["world"] = 7;
  auto it_query = ART.find("hello");
  ASSERT_NE(it_query, ART.end());
  EXPECT_EQ(it_query->second, 8);
  it_query = ART.find("world");
  ASSERT_NE(it_query, ART.end());
  EXPECT_EQ(it_query->second, 7);

  ART.NewAffineScope();
  // should create a default table for the new scope
  EXPECT_EQ(ART.num_scopes(), 2);
  EXPECT_EQ(ART.num_tables(), 1);
  // new scope should inherit old table at the beginning
  it_query = ART.find("hello");
  ASSERT_NE(it_query, ART.end());
  EXPECT_EQ(it_query->second, 8);
  it_query = ART.find("world");
  ASSERT_NE(it_query, ART.end());
  EXPECT_EQ(it_query->second, 7);
  // should copy a new table
  ART["hello"] = 9;
  ART["foo"] = 12;
  it_query = ART.find("hello");
  ASSERT_NE(it_query, ART.end());
  EXPECT_EQ(it_query->second, 9);
  it_query = ART.find("foo");
  ASSERT_NE(it_query, ART.end());
  EXPECT_EQ(it_query->second, 12);
  ART.CloseAffineScope<>(
    [&](affine_table_type& OutTable, const std::vector<table_type*>& Tables) {
      EXPECT_EQ(Tables.size(), 1);
      OutTable["world"] = 100;
    }
  );
  // should switch back to old table
  EXPECT_EQ(ART.num_scopes(), 1);
  it_query = ART.find("hello");
  ASSERT_NE(it_query, ART.end());
  EXPECT_EQ(it_query->second, 8);
  it_query = ART.find("world");
  ASSERT_NE(it_query, ART.end());
  EXPECT_EQ(it_query->second, 100);

  ART.NewAffineScope();
  ART["hello"] = 87;
  ART.NewBranch();
  // sibilin branches should not inteference each other
  it_query = ART.find("hello");
  ASSERT_NE(it_query, ART.end());
  EXPECT_EQ(it_query->second, 8);

  ART["hello"] = 94;
  ART.NewAffineScope();
  // should inhereit from the second branch
  it_query = ART.find("hello");
  ASSERT_NE(it_query, ART.end());
  EXPECT_EQ(it_query->second, 94);
}

TEST(ParserUnitTest, ParseComputation) {
  std::stringstream SS;
  // simple global var access
  SS << "main\n"
     << "var foo;\n"
     << "array[8][7] bar;\n"
     << "function yee;\n"
     << "{ return bar[0][1] + foo };\n"
     << "{ let foo <- 8 }.";
  Graph G;
  Parser P(SS, G);
  ASSERT_TRUE(P.Parse());

  std::ofstream OF("TestTopComputation1.dot");
  G.dumpGraphviz(OF);
}

TEST(ParserUnitTest, ParseInterprocedure) {
  std::stringstream SS;
  {
    // interprocedure global var accesses
    SS << "main\n"
       << "var foo;\n"
       << "array[9][4] bar;\n"
       << "function yee(a); {\n"
       << " let bar[0][a] <- a;\n"
       << " return bar[0][a]\n"
       << "};\n"
       << "function yoo;\n"
       << "{ return foo + 1 };\n"
       << "{ let foo <- call yee(2) * call yoo }.";
    Graph G;
    Parser P(SS, G);
    ASSERT_TRUE(P.Parse());

    std::ofstream OF("TestInterprocedure1.dot");
    G.dumpGraphviz(OF);
  }
  SS.clear();
}
