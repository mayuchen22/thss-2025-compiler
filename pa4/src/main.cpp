#include "ANTLRInputStream.h"
#include "CalculatorLexer.h"
#include "CalculatorParser.h"
#include "InterpreterVisitor.h"
#include "ParseTree.h"

#include <fstream>
#include <iostream>

using namespace antlr4;

int main(int argc, const char *argv[]) {
  if (argc < 2) {
    std::cerr << "Usage: interpreter <input-file>" << std::endl;
    return 1;
  }

  std::ifstream stream;
  stream.open(argv[1]);
  if (!stream.is_open()) {
    std::cerr << "Error: Cannot open file " << argv[1] << std::endl;
    return 1;
  }
  ANTLRInputStream input(stream);

  CalculatorLexer lexer(&input);
  CommonTokenStream tokens(&lexer);
  CalculatorParser parser(&tokens);

  try {
    tree::ParseTree *tree = parser.program();

    InterpreterVisitor visitor;
    visitor.visit(tree);

    if (visitor.hasExecutionError()) {
      return 1;
    }
  } catch (const std::exception& e) {
    std::cerr << "Error: " << e.what() << std::endl;
    return 1;
  }

  // TODO: Create your visitor and interpret the parse tree
  return 0;
}
