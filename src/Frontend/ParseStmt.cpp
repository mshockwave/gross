#include "gross/Support/Log.h"
#include "gross/Graph/Graph.h"
#include "gross/Graph/NodeUtils.h"
#include "Parser.h"

using namespace gross;

Node* Parser::ParseAssignment() {
  auto Tok = CurTok();
  if(Tok != Lexer::TOK_LET) {
    Log::E() << "Expecting 'let' keyword here\n";
    return nullptr;
  }
  (void) NextTok();

  auto* DesigNode = ParseDesignator();
  NodeProperties<IrOpcode::VirtSrcDesigAccess> DNP(DesigNode);
  if(!DNP) return nullptr;
  Tok = CurTok();

  if(Tok != Lexer::TOK_L_ARROW) {
    Log::E() << "Expecting '<-' here\n";
    return nullptr;
  }
  (void) NextTok();

  auto* ExprNode = ParseExpr();
  if(!ExprNode) return nullptr;

  auto* AssignNode = NodeBuilder<IrOpcode::SrcAssignStmt>(&G)
                     .Dest(DesigNode).Src(ExprNode)
                     .Build();
  assert(AssignNode && "fail to build SrcAssignStmt node");
  // update last modified map
  LastModified[DNP.decl()] = AssignNode;
  return AssignNode;
}

