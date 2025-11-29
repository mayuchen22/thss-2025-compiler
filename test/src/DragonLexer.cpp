#include "DragonLexer.hpp"
#include "Token.hpp"
#include <cctype>
#include <string>

DragonLexer::DragonLexer(const std::string &input)
  : Lexer(input) {}

Token DragonLexer::nextToken() {
  if (peek == EOF_CHAR) {
    return Token(TokenType::EOF_T, "EOF", line);
  }

  if (std::isspace(peek)) {
    return WS();
  }

  // TODO: finish me. You need to consider when to call ID()
  if (std::isalpha(peek)) {
    return ID();
  }
  
  if (std::isdigit(peek)) {
    return NUMBER();
  }

  int cur = peek;
  switch (cur) {
    // TODO: finish me. You need to consider EQ, NE, LT, LE, GT, GE
    case '(':
      advance(); return Token(TokenType::L_PAREN, "(", line);
    case ')':
      advance(); return Token(TokenType::R_PAREN, ")", line);
    case '{':
      advance(); return Token(TokenType::L_BRACE, "{", line);
    case '}':
      advance(); return Token(TokenType::R_BRACE, "}", line);
    case '[':
      advance(); return Token(TokenType::L_BRACKET, "[", line);
    case ']':
      advance(); return Token(TokenType::R_BRACKET, "]", line);
    case '=':
      advance(); return Token(TokenType::EQ, "=", line);
    case '<':
      advance();
      if (peek == '>') {
        advance(); return Token(TokenType::NE, "<>", line);
      }else if (peek == '=') {
        advance(); return Token(TokenType::LE, "<=", line);
      }else{
        return Token(TokenType::LT, "<", line);
      }
    case '>':
      advance();
      if (peek == '=') {
        advance(); return Token(TokenType::GE, ">=", line);
      }else{
        return Token(TokenType::GT, ">", line);
      }
    case ',':
      advance(); return Token(TokenType::COMMA, ",", line);
    case ';':
      advance(); return Token(TokenType::SEMI, ";", line);
    case '+':
      advance(); return Token(TokenType::PLUS, "+", line);
    case '-':
      advance(); return Token(TokenType::MINUS, "-", line);
    case '*':
      advance(); return Token(TokenType::MUL, "*", line);
    case '/':
      advance(); return Token(TokenType::DIV, "/", line);
    case '.':
      advance(); return Token(TokenType::DOT, ".", line);
    case '\'':
      advance(); return Token(TokenType::SQUOTE, "\'", line);
    default:
      char ch = static_cast<char>(peek);
      std::string s(1, ch);
      advance();
      return Token(TokenType::UNKNOWN, s, line);
  }
}

Token DragonLexer::WS() {
  // TODO: finish me. The WS() method should consume all whitespace characters (spaces, tabs, newlines)
  int startLine = line;
  while (std::isspace(peek)) {
    advance();
  }
  return Token(TokenType::WS, " ", startLine);
}

Token DragonLexer::ID() {
  // TODO: finish me. The ID() method should consume an identifier or keyword.
  std::string buffer;
  int startLine = line;
  while (std::isalnum(peek)) {
    buffer += static_cast<char>(peek);
    advance();
  }

  // Check if the buffer is a keyword
  static KeywordTable keywordTable;
  Token keywordToken = keywordTable.getKeyword(buffer);

  return Token(keywordToken.type, buffer, startLine);
}

Token DragonLexer::NUMBER() {
  // TODO: finish me. The NUMBER() method should consume an integer, real number, or scientific notation number.
  std::string buffer;
  int startLine = line;

  // Consume integer part
  while (std::isdigit(peek)) {
    buffer += static_cast<char>(peek);
    advance();

    // Check if only one apostrophe exists
    if (peek == '\'') {
      int savePose = pos;
      advance();
      if (!std::isdigit(peek)) {
        // only one apostrophe allowed
        resetPos(savePose);
      }else{
        buffer += '\'';
        continue;
      }
    }
  }

  bool isReal = false;
  bool isSci = false;

  // Check for decimal point
  if (peek == '.') {
    // Check if only one dot exists
    int savePose = pos;
    advance();
    if (std::isdigit(peek)) {
      isReal = true;
      buffer += '.';
      while (std::isdigit(peek)) {
        buffer += static_cast<char>(peek);
        advance();

        // Check if only one apostrophe exists
        if (peek == '\'') {
          int savePose = pos;
          advance();
          if (!std::isdigit(peek)) {
            // only one apostrophe allowed
            resetPos(savePose);
          }else{
            buffer += '\'';
            continue;
          }
        }
      }
    }else{
      // only one dot allowed
      resetPos(savePose);
    }
  }

  if (peek == 'E' || peek == 'e'){
    // Check if only one E/e exists
    int savePose = pos;
    std::string sciPart;
    sciPart += static_cast<char>(peek);
    advance();
    if (peek == '+' || peek == '-'){
      sciPart += static_cast<char>(peek);
      advance();
    }
    if (std::isdigit(peek)){
      isSci = true;
      while (std::isdigit(peek)) {
        sciPart += static_cast<char>(peek);
        advance();

        // Check if only one apostrophe exists
        if (peek == '\'') {
          int savePose = pos;
          advance();
          if (!std::isdigit(peek)) {
            // only one apostrophe allowed
            resetPos(savePose);
          }else{
            sciPart += '\'';
            continue;
          }
        }
      }
      buffer += sciPart;
    }else{
      // only one E/e allowed
      resetPos(savePose);
    }
  }

  if (isSci) {
    return Token(TokenType::SCI, buffer, startLine);
  } else if (isReal) {
    return Token(TokenType::REAL, buffer, startLine);
  } else {
    return Token(TokenType::INT, buffer, startLine);
  }
}
