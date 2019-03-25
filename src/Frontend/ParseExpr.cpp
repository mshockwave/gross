#include "gross/Support/Log.h"
#include "gross/Graph/Graph.h"
#include "gross/Graph/NodeUtils.h"
#include "Parser.h"
#include <cstdlib>

using namespace gross;

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
  // global scalar access
  if(G.IsGlobalVar(Decl)) {
    return ParseGlobalVarAccessDesignator(Decl, EffectNode);
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

  if(!Effect) {
    // create initial state
    Effect = getInitialValue(DeclNode);
    LastModified[DeclNode] = Effect;
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

  auto* AANode = AAB.Build();
  LastMemAccess[Effect].insert(AANode);

  return AANode;
}

Node* Parser::ParseGlobalVarAccessDesignator(Node* DeclNode, Node* Effect) {
  assert(NodeProperties<IrOpcode::SrcArrayDecl>(DeclNode));
  if(!Effect) {
    // create initial state
    Effect = getInitialValue(DeclNode);
    LastModified[DeclNode] = Effect;
  }

  auto* AANode = NodeBuilder<IrOpcode::SrcArrayAccess>(&G)
                 .Decl(DeclNode)
                 .Effect(Effect)
                 .AppendAccessDim(
                   NodeBuilder<IrOpcode::ConstantInt>(&G, 0).Build()
                  ).Build();
  LastMemAccess[Effect].insert(AANode);

  return AANode;
}

Node* Parser::ParseFactor() {
  Node* FN = nullptr;
  switch(CurTok()) {
  case Lexer::TOK_NUMBER: {
    auto val
      = static_cast<int32_t>(std::atoi(TokBuffer().c_str()));
    FN = NodeBuilder<IrOpcode::ConstantInt>(&G, val).Build();
    (void) NextTok();
    break;
  }
  case Lexer::TOK_L_PARAN: {
    (void) NextTok();
    FN = ParseExpr();
    if(CurTok() != Lexer::TOK_R_PARAN) {
      Log::E() << "expecting close paran\n";
      return nullptr;
    }
    (void) NextTok();
    break;
  }
  case Lexer::TOK_IDENT: {
    FN = ParseDesignator();
    break;
  }
  case Lexer::TOK_CALL: {
    FN = ParseFuncCall();
    break;
  }
  default:
    Log::E() << "Unexpecting \"" << TokBuffer() << "\""
             << " for factor\n";
    return nullptr;
  }

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

