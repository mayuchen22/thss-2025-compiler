#pragma once

#include "CalculatorBaseVisitor.h"
#include "SymbolTable.h"
#include <memory>
#include <sstream>
#include <iostream>
#include <any>

class InterpreterError : public std::exception {
private:
    std::string message;
    
public:
    InterpreterError(const std::string& msg) : message(msg) {}
    const char* what() const noexcept override { return message.c_str(); }
};

class InterpreterVisitor : public CalculatorBaseVisitor {
private:
    SymbolTable symbolTable;
    bool hasError = false;
    
    void reportError(const std::string& symbol, size_t line);
    int evaluateExpression(CalculatorParser::ExprContext* ctx);

public:
    virtual std::any visitProgram(CalculatorParser::ProgramContext *ctx) override;
    virtual std::any visitAssignment(CalculatorParser::AssignmentContext *ctx) override;
    virtual std::any visitBlock(CalculatorParser::BlockContext *ctx) override;
    virtual std::any visitPrintStmt(CalculatorParser::PrintStmtContext *ctx) override;
    virtual std::any visitParenExpr(CalculatorParser::ParenExprContext *ctx) override;
    virtual std::any visitLvalExpr(CalculatorParser::LvalExprContext *ctx) override;
    virtual std::any visitIntExpr(CalculatorParser::IntExprContext *ctx) override;
    virtual std::any visitMulExpr(CalculatorParser::MulExprContext *ctx) override;
    virtual std::any visitPlusExpr(CalculatorParser::PlusExprContext *ctx) override;
    
    bool hasExecutionError() const { return hasError; }
};