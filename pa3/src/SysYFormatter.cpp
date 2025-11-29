#include "SysYFormatter.h"
#include <iostream>

void SysYFormatter::addIndent() {
  for (int i = 0; i < indentLevel; ++i) code << "  ";
}
void SysYFormatter::addSpace() { code << " "; }
void SysYFormatter::addNewline() { code << "\n"; }
void SysYFormatter::formatBinaryOp(antlr4::tree::ParseTree* left, const std::string& op, antlr4::tree::ParseTree* right) {
  visit(left);
  addSpace();
  code << op;
  addSpace();
  visit(right);
}

std::any SysYFormatter::visitCompUnit(SysYParser::CompUnitContext *ctx) {
  bool firstElement = true;
  for (auto child : ctx->children) {
    if (child->getText() != "<EOF>") {
      // 除第一行外，每个函数前有一个空行
      if (!firstElement && dynamic_cast<SysYParser::FuncDefContext*>(child)) {
        addNewline();
      }
      firstElement = false;
      visit(child);
    }
  }
  return code.str();
}

std::any SysYFormatter::visitDecl(SysYParser::DeclContext *ctx) { return visitChildren(ctx); }

std::any SysYFormatter::visitConstDecl(SysYParser::ConstDeclContext *ctx) {
  code << "const";
  addSpace();
  visit(ctx->bType());
  addSpace();
  bool first = true;
  for (auto def : ctx->constDef()) {
    if (!first) { code << ","; addSpace(); }
    first = false;
    visit(def);
  }
  code << ";";
  addNewline();
  return nullptr;
}

std::any SysYFormatter::visitBType(SysYParser::BTypeContext *ctx) {
  code << "int";
  return nullptr;
}

std::any SysYFormatter::visitConstDef(SysYParser::ConstDefContext *ctx) {
  code << ctx->IDENT()->getText();
  for (auto exp : ctx->constExp()) {
    code << "[";
    visit(exp);
    code << "]";
  }
  code << " = ";
  visit(ctx->constInitVal());
  return nullptr;
}

std::any SysYFormatter::visitConstInitVal(SysYParser::ConstInitValContext *ctx) {
  if (ctx->constExp()) {
    visit(ctx->constExp());
  } else {
    code << "{";
    if (!ctx->constInitVal().empty()) {
      bool first = true;
      for (auto v : ctx->constInitVal()) {
        if (!first) { code << ","; addSpace(); }
        first = false;
        visit(v);
      }
    }
    code << "}";
  }
  return nullptr;
}

std::any SysYFormatter::visitVarDecl(SysYParser::VarDeclContext *ctx) {
  visit(ctx->bType());
  addSpace();
  bool first = true;
  for (auto def : ctx->varDef()) {
    if (!first) { code << ","; addSpace(); }
    first = false;
    visit(def);
  }
  code << ";";
  addNewline();
  return nullptr;
}

std::any SysYFormatter::visitVarDef(SysYParser::VarDefContext *ctx) {
  code << ctx->IDENT()->getText();
  for (auto exp : ctx->constExp()) {
    code << "[";
    visit(exp);
    code << "]";
  }
  if (ctx->ASSIGN()) {
    code << " = ";
    visit(ctx->initVal());
  }
  return nullptr;
}

std::any SysYFormatter::visitInitVal(SysYParser::InitValContext *ctx) {
  if (ctx->exp()) {
    visit(ctx->exp());
  } else {
    code << "{";
    if (!ctx->initVal().empty()) {
      bool first = true;
      for (auto v : ctx->initVal()) {
        if (!first) { code << ","; addSpace(); }
        first = false;
        visit(v);
      }
    }
    code << "}";
  }
  return nullptr;
}

std::any SysYFormatter::visitFuncDef(SysYParser::FuncDefContext *ctx) {
  // 移除原来的 lastWasFunction 逻辑，改用 visitCompUnit 中的统一处理
  
  visit(ctx->funcType());
  addSpace();
  code << ctx->IDENT()->getText();
  code << "(";
  if (ctx->funcFParams()) visit(ctx->funcFParams());
  code << ")";

  inFunction = true;

  // K&R: 函数头与 { 同行
  addSpace();
  suppressNextBlockIndent = true;
  visit(ctx->block());
  addNewline();

  inFunction = false;
  return nullptr;
}

std::any SysYFormatter::visitFuncType(SysYParser::FuncTypeContext *ctx) {
  if (ctx->VOID()) code << "void"; else code << "int";
  return nullptr;
}

std::any SysYFormatter::visitFuncFParams(SysYParser::FuncFParamsContext *ctx) {
  bool first = true;
  for (auto p : ctx->funcFParam()) {
    if (!first) { code << ","; addSpace(); }
    first = false;
    visit(p);
  }
  return nullptr;
}

std::any SysYFormatter::visitFuncFParam(SysYParser::FuncFParamContext *ctx) {
  visit(ctx->bType());
  addSpace();
  code << ctx->IDENT()->getText();
  if (!ctx->L_BRACKT().empty()) {
    code << "[]";
    for (int i = 1; i < ctx->L_BRACKT().size(); ++i) {
      code << "[";
      visit(ctx->exp(i - 1));
      code << "]";
    }
  }
  return nullptr;
}

std::any SysYFormatter::visitBlock(SysYParser::BlockContext *ctx) {
  bool prevInBlock = inBlock;
  inBlock = true;

  // 区分两种抑制来源：control header vs parent blockItem
  bool suppressedByControl = false;
  bool suppress = false;
  if (suppressNextBlockIndent) {
    suppress = true;
    suppressedByControl = true;
    suppressNextBlockIndent = false;
  } else if (suppressNextBlockIndentFromBlockItem) {
    suppress = true;
    suppressedByControl = false;
    suppressNextBlockIndentFromBlockItem = false;
  }

  if (!suppress) addIndent();
  code << "{";
  addNewline();

  indentLevel++;

  for (auto item : ctx->blockItem()) {
    // 在每个 blockItem 前由父 block 统一打印缩进
    addIndent();

    // 如果 item 本身是 stmt 且该 stmt 为 block（即 nested block），
    // 标记下次遇到 block 的缩进应该被抑制（父子避免双重缩进）
    if (item->stmt() && item->stmt()->block()) {
      suppressNextBlockIndentFromBlockItem = true;
    }

    visit(item);
    // 大多数 visit(item) 会在结尾 addNewline()
  }

  indentLevel--;

  addIndent();
  code << "}";

  // 换行策略：
  // - 如果这是最外层/普通 block（即没有被 control header 抑制），并且我们处在一个包裹块内，
  //   则闭括号后应该换行，避免 "} if(...)" 黏在一起。
  // - 如果被 control header 抑制 (K&R)，则不要在闭括号后换行；由调用方决定何时换行。
  if (prevInBlock) {
    if (!suppress || (suppress && !suppressedByControl)) {
      addNewline();
    }
  }

  inBlock = prevInBlock;
  return nullptr;
}

std::any SysYFormatter::visitBlockItem(SysYParser::BlockItemContext *ctx) {
  return visitChildren(ctx);
}

std::any SysYFormatter::visitStmt(SysYParser::StmtContext *ctx) {
  // IMPORTANT: 不在这里 addIndent() —— visitBlock 在作为 blockItem 时会在调用本函数之前已经 addIndent()

  if (ctx->RETURN()) {
    code << "return";
    if (ctx->exp()) { addSpace(); visit(ctx->exp()); }
    code << ";";
    addNewline();
    return nullptr;
  }

  if (ctx->lVal() && ctx->ASSIGN()) {
    visit(ctx->lVal());
    code << " = ";
    visit(ctx->exp());
    code << ";";
    addNewline();
    return nullptr;
  }

  if (ctx->exp() && ctx->SEMICOLON() && !ctx->lVal()) {
      visit(ctx->exp());
      code << ";";
      addNewline();
      return nullptr;
    }
  
    // 专门处理空语句：只有 SEMICOLON 没有 exp
  if (ctx->SEMICOLON() && !ctx->exp() && !ctx->lVal() && !ctx->RETURN() && 
        !ctx->IF() && !ctx->WHILE() && !ctx->BREAK() && !ctx->CONTINUE() && !ctx->block()) {
      code << ";";
      addNewline();
      return nullptr;
  }

  if (ctx->block()) {
    // 直接调用 nested block；visitBlock 会根据 suppressNextBlockIndent{FromBlockItem,Control} 决定
    visit(ctx->block());
    return nullptr;
  }

  if (ctx->IF()) {
    code << "if (";
    visit(ctx->cond());
    code << ")";

    auto thenStmt = ctx->stmt(0);
    bool thenIsBlock = thenStmt->block() != nullptr;

    if (thenIsBlock) {
      addSpace();
      // control header 情形，下一次 block 要与当前同一行 (K&R)
      suppressNextBlockIndent = true;
      visit(thenStmt);

      // 无论是否有 else，then-block 结束后都需要换行
      addNewline();
    } else {
      addNewline();
      indentLevel++;
      addIndent();
      visit(thenStmt);
      indentLevel--;
    }

    if (ctx->ELSE()) {
      auto elseStmt = ctx->stmt(1);
      bool elseIsBlock = elseStmt->block() != nullptr;
      bool elseIsIf = elseStmt->IF() != nullptr;

      // else 和 else if 总是在新的一行开始
      addIndent();
      code << "else";
      
      if (elseIsIf) {
        // else if 情况：在新行开始，然后继续处理 if
        addSpace();
        visit(elseStmt);
      } else if (elseIsBlock) {
        // else { ... } 情况
        addSpace();
        suppressNextBlockIndent = true;
        visit(elseStmt);
        addNewline();
      } else {
        // 单行 else 语句
        addNewline();
        indentLevel++;
        addIndent();
        visit(elseStmt);
        indentLevel--;
      }
    }
    return nullptr;
  }

  if (ctx->WHILE()) {
    code << "while (";
    visit(ctx->cond());
    code << ")";

    auto body = ctx->stmt(0);
    bool bodyIsBlock = body->block() != nullptr;

    if (bodyIsBlock) {
      addSpace();
      suppressNextBlockIndent = true;
      visit(body);
      // body 是 block，打印完 '}' 后需要换行以隔离后续语句
      addNewline();
    } else {
      addNewline();
      indentLevel++;
      addIndent();
      visit(body);
      indentLevel--;
    }
    return nullptr;
  }

  if (ctx->BREAK()) {
    code << "break;";
    addNewline();
    return nullptr;
  }
  if (ctx->CONTINUE()) {
    code << "continue;";
    addNewline();
    return nullptr;
  }

  return nullptr;
}

// ---- expressions (unchanged except spacing rules) ----

std::any SysYFormatter::visitExp(SysYParser::ExpContext *ctx) {
  if (ctx->L_PAREN() && ctx->exp().size() == 1) { code << "("; visit(ctx->exp(0)); code << ")"; return nullptr; }
  if (ctx->lVal()) return visit(ctx->lVal());
  if (ctx->number()) return visit(ctx->number());
  if (ctx->IDENT() && ctx->L_PAREN()) {
    code << ctx->IDENT()->getText() << "(";
    if (ctx->funcRParams()) visit(ctx->funcRParams());
    code << ")";
    return nullptr;
  }
  if (ctx->unaryOp() && ctx->exp().size() == 1) { visit(ctx->unaryOp()); visit(ctx->exp(0)); return nullptr; }
  if (ctx->exp().size() == 2) {
    visit(ctx->exp(0)); addSpace();
    if (ctx->MUL()) code << "*";
    else if (ctx->DIV()) code << "/";
    else if (ctx->MOD()) code << "%";
    else if (ctx->PLUS()) code << "+";
    else if (ctx->MINUS()) code << "-";
    else if (ctx->LT()) code << "<";
    else if (ctx->GT()) code << ">";
    else if (ctx->LE()) code << "<=";
    else if (ctx->GE()) code << ">=";
    else if (ctx->EQ()) code << "==";
    else if (ctx->NEQ()) code << "!=";
    else if (ctx->AND()) code << "&&";
    else if (ctx->OR()) code << "||";
    addSpace(); visit(ctx->exp(1));
    return nullptr;
  }
  return visitChildren(ctx);
}

std::any SysYFormatter::visitCond(SysYParser::CondContext *ctx) {
  if (ctx->exp()) return visit(ctx->exp());
  if (ctx->cond().size() == 2) {
    visit(ctx->cond(0)); addSpace();
    if (ctx->LT()) code << "<";
    else if (ctx->GT()) code << ">";
    else if (ctx->LE()) code << "<=";
    else if (ctx->GE()) code << ">=";
    else if (ctx->EQ()) code << "==";
    else if (ctx->NEQ()) code << "!=";
    else if (ctx->AND()) code << "&&";
    else if (ctx->OR()) code << "||";
    addSpace(); visit(ctx->cond(1));
  }
  return nullptr;
}

std::any SysYFormatter::visitLVal(SysYParser::LValContext *ctx) {
  code << ctx->IDENT()->getText();
  for (auto e : ctx->exp()) { code << "["; visit(e); code << "]"; }
  return nullptr;
}

std::any SysYFormatter::visitNumber(SysYParser::NumberContext *ctx) {
  code << ctx->INTEGER_CONST()->getText();
  return nullptr;
}

std::any SysYFormatter::visitUnaryExp(SysYParser::UnaryExpContext *ctx) {
  if (ctx->primaryExp()) visit(ctx->primaryExp());
  else if (ctx->IDENT() && ctx->L_PAREN()) {
    code << ctx->IDENT()->getText() << "(";
    if (ctx->funcRParams()) visit(ctx->funcRParams());
    code << ")";
  } else if (ctx->unaryOp() && ctx->unaryExp()) {
    visit(ctx->unaryOp()); visit(ctx->unaryExp());
  }
  return nullptr;
}

std::any SysYFormatter::visitUnaryOp(SysYParser::UnaryOpContext *ctx) {
  if (ctx->PLUS()) code << "+";
  else if (ctx->MINUS()) code << "-";
  else if (ctx->NOT()) code << "!";
  return nullptr;
}

std::any SysYFormatter::visitFuncRParams(SysYParser::FuncRParamsContext *ctx) {
  bool first = true;
  for (auto e : ctx->exp()) {
    if (!first) { code << ","; addSpace(); }
    first = false;
    visit(e);
  }
  return nullptr;
}

std::any SysYFormatter::visitMulExp(SysYParser::MulExpContext *ctx) {
  if (ctx->mulExp()) {
    visit(ctx->mulExp()); addSpace();
    if (ctx->MUL()) code << "*";
    else if (ctx->DIV()) code << "/";
    else if (ctx->MOD()) code << "%";
    addSpace(); visit(ctx->unaryExp());
  } else visit(ctx->unaryExp());
  return nullptr;
}

std::any SysYFormatter::visitAddExp(SysYParser::AddExpContext *ctx) {
  if (ctx->addExp()) {
    visit(ctx->addExp()); addSpace();
    if (ctx->PLUS()) code << "+";
    else if (ctx->MINUS()) code << "-";
    addSpace(); visit(ctx->mulExp());
  } else visit(ctx->mulExp());
  return nullptr;
}

std::any SysYFormatter::visitRelExp(SysYParser::RelExpContext *ctx) {
  if (ctx->relExp()) {
    visit(ctx->relExp()); addSpace();
    if (ctx->LT()) code << "<";
    else if (ctx->GT()) code << ">";
    else if (ctx->LE()) code << "<=";
    else if (ctx->GE()) code << ">=";
    addSpace(); visit(ctx->addExp());
  } else visit(ctx->addExp());
  return nullptr;
}

std::any SysYFormatter::visitEqExp(SysYParser::EqExpContext *ctx) {
  if (ctx->eqExp()) {
    visit(ctx->eqExp()); addSpace();
    if (ctx->EQ()) code << "==";
    else if (ctx->NEQ()) code << "!=";
    addSpace(); visit(ctx->relExp());
  } else visit(ctx->relExp());
  return nullptr;
}

std::any SysYFormatter::visitLAndExp(SysYParser::LAndExpContext *ctx) {
  if (ctx->lAndExp()) {
    visit(ctx->lAndExp()); addSpace();
    code << "&&"; addSpace(); visit(ctx->eqExp());
  } else visit(ctx->eqExp());
  return nullptr;
}

std::any SysYFormatter::visitLOrExp(SysYParser::LOrExpContext *ctx) {
  if (ctx->lOrExp()) {
    visit(ctx->lOrExp()); addSpace();
    code << "||"; addSpace(); visit(ctx->lAndExp());
  } else visit(ctx->lAndExp());
  return nullptr;
}

std::any SysYFormatter::visitConstExp(SysYParser::ConstExpContext *ctx) { return visit(ctx->addExp()); }

std::any SysYFormatter::visitPrimaryExp(SysYParser::PrimaryExpContext *ctx) {
  if (ctx->L_PAREN() && ctx->exp()) { code << "("; visit(ctx->exp()); code << ")"; }
  else if (ctx->lVal()) visit(ctx->lVal());
  else if (ctx->number()) visit(ctx->number());
  return nullptr;
}
