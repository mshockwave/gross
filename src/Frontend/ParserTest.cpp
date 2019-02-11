#include "Parser.h"
#include "gtest/gtest.h"
#include <string>

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
