#include "InterpreterVisitor.h"
#include "CalculatorParser.h"

using namespace antlr4;

void InterpreterVisitor::reportError(const std::string& symbol, size_t line) {
    std::cout << "Error: Undefined symbol '" << symbol << "' at line " << line << std::endl;
    hasError = true;
}

std::any InterpreterVisitor::visitProgram(CalculatorParser::ProgramContext *ctx) {
    // Visit all statements in order
    for (auto statement : ctx->statement()) {
        visit(statement);
        if (hasError) break;
    }
    return nullptr;
}

std::any InterpreterVisitor::visitAssignment(CalculatorParser::AssignmentContext *ctx) {
    if (hasError) return nullptr;
    
    std::string varName = ctx->ID()->getText();
    auto value = visit(ctx->expr());
    
    if (hasError) {
        return nullptr;
    }
    
    int intValue = std::any_cast<int>(value);
    if (!symbolTable.assign(varName, intValue)) {
        symbolTable.define(varName, intValue);
    }
    
    return nullptr;
}

std::any InterpreterVisitor::visitBlock(CalculatorParser::BlockContext *ctx) {
    if (hasError) return nullptr;
    
    symbolTable.enterScope();
    
    for (auto statement : ctx->statement()) {
        visit(statement);
        if (hasError) break;
    }
    
    symbolTable.exitScope();
    return nullptr;
}

std::any InterpreterVisitor::visitPrintStmt(CalculatorParser::PrintStmtContext *ctx) {
    if (hasError) return nullptr;
    
    auto value = visit(ctx->expr());
    if (!hasError) {
        std::cout << std::any_cast<int>(value) << std::endl;
    }
    return nullptr;
}

std::any InterpreterVisitor::visitParenExpr(CalculatorParser::ParenExprContext *ctx) {
    return visit(ctx->expr());
}

std::any InterpreterVisitor::visitLvalExpr(CalculatorParser::LvalExprContext *ctx) {
    std::string varName = ctx->ID()->getText();
    int value;
    if (symbolTable.lookup(varName, value)) {
        return value;
    } else {
        reportError(varName, ctx->getStart()->getLine());
        return 0;
    }
}

std::any InterpreterVisitor::visitIntExpr(CalculatorParser::IntExprContext *ctx) {
    return std::stoi(ctx->INT()->getText());
}

std::any InterpreterVisitor::visitMulExpr(CalculatorParser::MulExprContext *ctx) {
    if (hasError) return 0;
    
    auto leftAny = visit(ctx->expr(0));
    if (hasError) return 0;
    auto rightAny = visit(ctx->expr(1));
    if (hasError) return 0;
    
    int left = std::any_cast<int>(leftAny);
    int right = std::any_cast<int>(rightAny);
    
    if (ctx->MUL()) {
        return left * right;
    } else if (ctx->DIV()) {
        if (right == 0) {
            std::cout << "Error: Division by zero at line " << ctx->getStart()->getLine() << std::endl;
            hasError = true;
            return 0;
        }
        return left / right;
    } else if (ctx->MOD()) {
        if (right == 0) {
            std::cout << "Error: Modulo by zero at line " << ctx->getStart()->getLine() << std::endl;
            hasError = true;
            return 0;
        }
        return left % right;
    }
    
    return 0;
}

std::any InterpreterVisitor::visitPlusExpr(CalculatorParser::PlusExprContext *ctx) {
    if (hasError) return 0;
    
    auto leftAny = visit(ctx->expr(0));
    if (hasError) return 0;
    auto rightAny = visit(ctx->expr(1));
    if (hasError) return 0;
    
    int left = std::any_cast<int>(leftAny);
    int right = std::any_cast<int>(rightAny);
    
    if (ctx->PLUS()) {
        return left + right;
    } else if (ctx->MINUS()) {
        return left - right;
    }
    
    return 0;
}