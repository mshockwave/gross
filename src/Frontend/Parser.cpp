#include "gross/Support/Log.h"
#include "gross/Graph/Graph.h"
#include "gross/Graph/NodeUtils.h"
#include "Parser.h"
#include <cstdlib>

using namespace gross;

/// Note: For each ParseXXX, it would expect the lexer cursor
/// starting on the first token of its semantic rule.
bool Parser::Parse() {
  auto Tok = NextTok();
  while(Tok != Lexer::TOK_EOF) {
    switch(Tok) {
    case Lexer::TOK_VAR:
      if(!ParseVarDecl<IrOpcode::SrcVarDecl>())
        return false;
      else
        break;
    case Lexer::TOK_ARRAY:
      if(!ParseVarDecl<IrOpcode::SrcArrayDecl>())
        return false;
      else
        break;
    case Lexer::TOK_FUNCTION:
    case Lexer::TOK_PROCEDURE:
      if(!ParseFuncDecl())
        return false;
      else
        break;
    default:
      gross_unreachable("Unimplemented");
    }
  }
  return true;
}

template<IrOpcode::ID OC>
bool Parser::ParseTypeDecl(NodeBuilder<OC>& NB) {
  gross_unreachable("Unimplemented");
  return false;
}
template<>
bool Parser::ParseTypeDecl(NodeBuilder<IrOpcode::SrcVarDecl>& NB) {
  (void) NextTok();
  return true;
}
template<>
bool Parser::ParseTypeDecl(NodeBuilder<IrOpcode::SrcArrayDecl>& NB) {
  Lexer::Token Tok;
  while(true) {
    Tok = NextTok();
    if(Tok != Lexer::TOK_L_BRACKET) break;
    (void) NextTok();
    Node* ExprNode = ParseExpr();
    NB.AddDim(ExprNode);
    Tok = CurTok();
    if(Tok != Lexer::TOK_R_BRACKET) {
      Log::E() << "Expecting close bracket\n";
      return false;
    }
  }
  return true;
}

template<IrOpcode::ID OC>
bool Parser::ParseVarDecl(std::vector<Node*>* Results) {
  NodeBuilder<OC> NB(&G);
  if(!ParseTypeDecl(NB)) return false;

  Lexer::Token Tok = CurTok();
  std::string SymName;
  while(true) {
    if(Tok != Lexer::TOK_IDENT) {
      Log::E() << "Expecting identifier\n";
      return false;
    }
    SymName = TokBuffer();
    SymbolLookup Lookup(*this, SymName);
    if(Lookup.InCurrentScope()) {
      Log::E() << "variable already declared in this scope\n";
      return false;
    }
    auto* VarDeclNode = NB.SetSymbolName(SymName)
                          .Build();
    CurSymTable().insert({SymName, VarDeclNode});
    if(Results) Results->push_back(VarDeclNode);

    Tok = NextTok();
    if(Tok == Lexer::TOK_SEMI_COLON)
      break;
    else if(Tok != Lexer::TOK_COMMA) {
      Log::E() << "Expecting semicolomn or comma\n";
      return false;
    }
    Tok = NextTok();
  }

  assert(Tok == Lexer::TOK_SEMI_COLON &&
         "Expecting semi-colon");
  (void) NextTok();
  return true;
}

Node* Parser::ParseFactor() {
  // TODO: designator and funcCall rule

  Node* FN = nullptr;
  switch(CurTok()) {
  case Lexer::TOK_NUMBER: {
    auto val
      = static_cast<int32_t>(std::atoi(TokBuffer().c_str()));
    FN = NodeBuilder<IrOpcode::ConstantInt>(&G, val).Build();
    break;
  }
  case Lexer::TOK_L_PARAN: {
    (void) NextTok();
    FN = ParseExpr();
    if(CurTok() != Lexer::TOK_R_PARAN) {
      Log::E() << "expecting close paran\n";
      return nullptr;
    }
    break;
  }
  default:
    Log::E() << "Unexpecting \"" << TokBuffer() << "\""
             << " for factor\n";
    return nullptr;
  }

  (void) NextTok();
  return FN;
}

Node* Parser::ParseTerm() {
  auto LHSFactor = ParseFactor();
  if(!LHSFactor) return nullptr;

  while(true) {
    if(Lex.isBinOpStar() || Lex.isBinOpSlash()) {
      auto Op = TokBuffer();
      (void) NextTok();
      auto* RHSFactor = ParseFactor();
      if(!RHSFactor) return nullptr;
      if(Op == "*") {
        LHSFactor = NodeBuilder<IrOpcode::BinMul>(&G)
                    .LHS(LHSFactor).RHS(RHSFactor)
                    .Build();
      } else if(Op == "/") {
        LHSFactor = NodeBuilder<IrOpcode::BinDiv>(&G)
                    .LHS(LHSFactor).RHS(RHSFactor)
                    .Build();
      } else {
        gross_unreachable("Unrecognized term op");
      }
    } else {
      break;
    }
  }

  return LHSFactor;
}

Node* Parser::ParseExpr() {
  auto LHSTerm = ParseTerm();
  if(!LHSTerm) return nullptr;

  while(true) {
    if(Lex.isBinOpPlus() || Lex.isBinOpMinus()) {
      auto Op = TokBuffer();
      (void) NextTok();
      auto* RHSTerm = ParseTerm();
      if(!RHSTerm) return nullptr;
      if(Op == "+") {
        LHSTerm = NodeBuilder<IrOpcode::BinAdd>(&G)
                    .LHS(LHSTerm).RHS(RHSTerm)
                    .Build();
      } else if(Op == "-") {
        LHSTerm = NodeBuilder<IrOpcode::BinSub>(&G)
                    .LHS(LHSTerm).RHS(RHSTerm)
                    .Build();
      } else {
        gross_unreachable("Unrecognized expr op");
      }
    } else {
      break;
    }
  }

  return LHSTerm;
}

Node* Parser::ParseRelation() {
  auto* LHSExpr = ParseExpr();
  if(!LHSExpr) return nullptr;

  auto Tok = CurTok();
  (void) NextTok();

  auto* RHSExpr = ParseExpr();
  if(!RHSExpr) return nullptr;
#define REL_OP_BUILDER(ROP, OC) \
  case Lexer::ROP:  \
    return NodeBuilder<IrOpcode::OC>(&G)  \
           .LHS(LHSExpr).RHS(RHSExpr) \
           .Build()

  switch(Tok) {
  default:
    return nullptr;
  REL_OP_BUILDER(TOK_REL_LE, BinLe);
  REL_OP_BUILDER(TOK_REL_LT, BinLt);
  REL_OP_BUILDER(TOK_REL_GE, BinGe);
  REL_OP_BUILDER(TOK_REL_GT, BinGt);
  REL_OP_BUILDER(TOK_REL_EQ, BinEq);
  REL_OP_BUILDER(TOK_REL_NE, BinNe);
  }
#undef REL_OP_BUILDER
}

Node* Parser::ParseDesignator() {
  auto Tok = CurTok();
  if(Tok != Lexer::TOK_IDENT) {
    Log::E() << "Expecting identifier\n";
    return nullptr;
  }

  auto IdentName = TokBuffer();
  SymbolLookup Lookup(*this, IdentName);
  Node* Decl = (*Lookup).first;
  if(!Decl) {
    Log::E() << "Symbol \'" << IdentName << "\' not declared\n";
    return nullptr;
  }

  // finding side effects
  Node* EffectNode = nullptr;
  auto ILM = LastModified.find(Decl);
  if(ILM != LastModified.end())
    EffectNode = ILM->second;

  // array access
  if(NextTok() == Lexer::TOK_L_BRACKET) {
    return ParseArrayAccessDesignator(Decl, EffectNode, IdentName);
  }

  // scalar access
  return NodeBuilder<IrOpcode::SrcVarAccess>(&G)
         .Decl(Decl).Effect(EffectNode)
         .Build();
}

Node* Parser::ParseArrayAccessDesignator(Node* DeclNode, Node* Effect,
                                         const std::string& IdentName) {
  if(!NodeProperties<IrOpcode::SrcArrayDecl>(DeclNode)) {
    Log::E() << "identifier \""
             << IdentName << "\""
             << " is not declared as array\n";
    return nullptr;
  }

  NodeBuilder<IrOpcode::SrcArrayAccess> AAB(&G);
  AAB.Decl(DeclNode)
     .Effect(Effect);

  while(true) {
    auto Tok = CurTok();
    if(Tok != Lexer::TOK_L_BRACKET) {
      Log::E() << "Expecting left bracket\n";
      return nullptr;
    }
    (void) NextTok();
    auto* Expr = ParseExpr();
    if(!Expr) return nullptr;
    Tok = CurTok();
    if(Tok != Lexer::TOK_R_BRACKET) {
      Log::E() << "Expecting right bracket\n";
      return nullptr;
    }
    AAB.AppendAccessDim(Expr);
    if(NextTok() != Lexer::TOK_L_BRACKET) break;
  }

  return AAB.Build();
}

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

bool Parser::ParseFuncDecl() {
  Lexer::Token Tok = NextTok();
  if(Tok != Lexer::TOK_IDENT) {
    Log::E() << "Expect identifier for function\n";
    return false;
  }
  const auto& FuncName = TokBuffer();
  // TODO: Create start node and register function
  Tok = NextTok();
  if(Tok == Lexer::TOK_L_PARAN) {
    // TODO: Argument list
    Tok = NextTok();
  }
  if(Tok != Lexer::TOK_SEMI_COLON) {
    Log::E() << "Expect ';' at the end\n";
    return false;
  }
  // TODO: Function body
  Tok = NextTok();
  if(Tok != Lexer::TOK_SEMI_COLON) {
    Log::E() << "Expect ';' at the end\n";
    return false;
  }
  return true;
}
