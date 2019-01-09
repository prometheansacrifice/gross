#include "gross/Support/Log.h"
#include "gross/Graph/Graph.h"
#include "gross/Graph/NodeUtils.h"
#include "Parser.h"

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
bool Parser::ParseVarDecl() {
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

Node* Parser::ParseExpr() {
  return nullptr;
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