#include "Parser.h"
#include "ParserTest.h"
#include "gross/Graph/Graph.h"
#include "gross/Graph/NodeUtils.h"
#include "gtest/gtest.h"
#include <algorithm>
#include <sstream>
#include <fstream>

using namespace gross;

TEST(ParserTest, TestAssignment) {
  std::stringstream SS;
  {
    // single assignment
    SS << "var megumin;"
       << "let megumin <- 2";
    Graph G;
    Parser P(SS, G);
    (void) P.getLexer().getNextToken();
    SetMockContext(P,G);
    ASSERT_TRUE(P.ParseVarDecl<IrOpcode::SrcVarDecl>());

    Node* Assign = P.ParseAssignment();
    NodeProperties<IrOpcode::SrcAssignStmt> NPA(Assign);
    ASSERT_TRUE(NPA);
    EXPECT_EQ(NodeProperties<IrOpcode::ConstantInt>(NPA.source())
              .as<int32_t>(G), 2);
    EXPECT_TRUE(NodeProperties<IrOpcode::SrcVarAccess>(NPA.dest()));
  }
  SS.clear();
  {
    // multiple assignments
    SS << "var megumin;"
       << "let megumin <- 2\n"
       << "let megumin <- 87";
    Graph G;
    Parser P(SS, G);
    (void) P.getLexer().getNextToken();
    SetMockContext(P,G);
    ASSERT_TRUE(P.ParseVarDecl<IrOpcode::SrcVarDecl>());

    Node* Assign1 = P.ParseAssignment();
    NodeProperties<IrOpcode::SrcAssignStmt> NPA1(Assign1);
    ASSERT_TRUE(NPA1);
    Node* Assign2 = P.ParseAssignment();
    NodeProperties<IrOpcode::SrcAssignStmt> NPA2(Assign2);
    ASSERT_TRUE(NPA2);
    EXPECT_EQ(NodeProperties<IrOpcode::ConstantInt>(NPA2.source())
              .as<int32_t>(G), 87);
    auto* Access2 = NPA2.dest();
    NodeProperties<IrOpcode::SrcVarAccess> NPAC2(Access2);
    ASSERT_TRUE(NPAC2);
    EXPECT_EQ(NPAC2.effect_dependency(), Assign1);
  }
  SS.clear();
  {
    // single array assignment
    SS << "array[94][87] megumin;"
       << "let megumin[1][5] <- 2";
    Graph G;
    Parser P(SS, G);
    (void) P.getLexer().getNextToken();
    SetMockContext(P,G);
    ASSERT_TRUE(P.ParseVarDecl<IrOpcode::SrcArrayDecl>());

    Node* Assign = P.ParseAssignment();

    NodeProperties<IrOpcode::SrcAssignStmt> NPA(Assign);
    ASSERT_TRUE(NPA);
    EXPECT_EQ(NodeProperties<IrOpcode::ConstantInt>(NPA.source())
              .as<int32_t>(G), 2);

    NodeProperties<IrOpcode::SrcArrayAccess> NPAC(NPA.dest());
    ASSERT_TRUE(NPAC);
    ASSERT_EQ(NPAC.dim_size(), 2);
    Node *Dim0 = NPAC.dim(0),
         *Dim1 = NPAC.dim(1);
    EXPECT_EQ(NodeProperties<IrOpcode::ConstantInt>(Dim0)
              .as<int32_t>(G), 1);
    EXPECT_EQ(NodeProperties<IrOpcode::ConstantInt>(Dim1)
              .as<int32_t>(G), 5);
  }
  SS.clear();
  {
    // multiple array assignments
    SS << "array[94][87] megumin;"
       << "let megumin[0][8] <- 20\n"
       << "let megumin[9][97] <- 50";
    Graph G;
    Parser P(SS, G);
    (void) P.getLexer().getNextToken();
    SetMockContext(P,G);
    ASSERT_TRUE(P.ParseVarDecl<IrOpcode::SrcArrayDecl>());

    Node* Assign1 = P.ParseAssignment();
    NodeProperties<IrOpcode::SrcAssignStmt> NPA1(Assign1);
    ASSERT_TRUE(NPA1);
    EXPECT_EQ(NodeProperties<IrOpcode::ConstantInt>(NPA1.source())
              .as<int32_t>(G), 20);

    Node* Assign2 = P.ParseAssignment();
    NodeProperties<IrOpcode::SrcAssignStmt> NPA2(Assign2);
    ASSERT_TRUE(NPA2);
    EXPECT_EQ(NodeProperties<IrOpcode::ConstantInt>(NPA2.source())
              .as<int32_t>(G), 50);

    auto* Access2 = NPA2.dest();
    NodeProperties<IrOpcode::SrcArrayAccess> NPAC2(Access2);
    ASSERT_TRUE(NPAC2);
    EXPECT_EQ(NPAC2.effect_dependency(), Assign1);
  }
}

TEST(ParserTest, TestIfStmt) {
  std::stringstream SS;
  {
    // PHI node without else block
    SS << "var foo;\n"
       << "let foo <- 0\n"
       << "if 1 < 2 then\n"
       << "  let foo <- 3\n"
       << "fi";
    Graph G;
    Parser P(SS, G);
    (void) P.getLexer().getNextToken();
    SetMockContext(P,G);
    ASSERT_TRUE(P.ParseVarDecl<IrOpcode::SrcVarDecl>());

    EXPECT_TRUE(P.ParseAssignment());

    auto* MergeNode = P.ParseIfStmt();
    NodeProperties<IrOpcode::Merge> MNP(MergeNode);
    ASSERT_TRUE(MNP);
    EXPECT_FALSE(MNP.FalseBranch());
    // check statements within true branch
    auto* TrueBr = MNP.TrueBranch();
    ASSERT_TRUE(TrueBr);
    auto CtrlUsers = TrueBr->control_users();
    auto ItCU = std::find_if(
      CtrlUsers.begin(), CtrlUsers.end(),
      [](Node* N)-> bool{ return NodeProperties<IrOpcode::SrcAssignStmt>(N); }
    );
    EXPECT_NE(ItCU, CtrlUsers.end());

    std::ofstream OF("TestIfStmt1.dot");
    G.dumpGraphviz(OF);
  }
  SS.clear();
  {
    // PHI node with two branches
    SS << "var foo;\n"
       << "if 1 < 2 then\n"
       << "  let foo <- 3\n"
       << "else\n"
       << "  let foo <- 4\n"
       << "fi";
    Graph G;
    Parser P(SS, G);
    (void) P.getLexer().getNextToken();
    SetMockContext(P,G);

    ASSERT_TRUE(P.ParseVarDecl<IrOpcode::SrcVarDecl>());

    auto* MergeNode = P.ParseIfStmt();
    ASSERT_TRUE(NodeProperties<IrOpcode::Merge>(MergeNode));
    auto MergeCtrlUsers = MergeNode->control_users();
    auto ItCU = std::find_if(
      MergeCtrlUsers.begin(), MergeCtrlUsers.end(),
      [](Node* N) -> bool { return NodeProperties<IrOpcode::Phi>(N); }
    );
    ASSERT_NE(ItCU, MergeCtrlUsers.end());
    Node* PN = *ItCU;
    EXPECT_EQ(PN->getNumEffectInput(), 2);
    for(Node* N : PN->effect_inputs())
      EXPECT_TRUE(NodeProperties<IrOpcode::SrcAssignStmt>(N));

    std::ofstream OF("TestIfStmt1-2.dot");
    G.dumpGraphviz(OF);
  }
  SS.clear();
  {
    // multiple expressions within branch region
    SS << "var foo;\n"
       << "if 1 < 2 then\n"
       << "  let foo <- 3;\n"
       << "  let foo <- 8\n"
       << "else\n"
       << "  let foo <- 4\n"
       << "fi";
    Graph G;
    Parser P(SS, G);
    (void) P.getLexer().getNextToken();
    SetMockContext(P,G);

    ASSERT_TRUE(P.ParseVarDecl<IrOpcode::SrcVarDecl>());

    auto* MergeNode = P.ParseIfStmt();
    ASSERT_TRUE(NodeProperties<IrOpcode::Merge>(MergeNode));

    std::ofstream OF("TestIfStmt1-3.dot");
    G.dumpGraphviz(OF);
  }
}
