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
    case '=':
      advance();
      return Token(TokenType::EQ, "=", line);
    case '<':
      advance();
      if (peek == '=') {
        advance();
        return Token(TokenType::LE, "<=", line);
      } else if (peek == '>') {
        advance();
        return Token(TokenType::NE, "<>", line);
      } else {
        return Token(TokenType::LT, "<", line);
      }
    case '>':
      advance();
      if (peek == '=') {
        advance();
        return Token(TokenType::GE, ">=", line);
      } else {
        return Token(TokenType::GT, ">", line);
      }
    case '\'':
      advance();
      return Token(TokenType::SQUOTE, "'", line);
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
    default:
      char ch = static_cast<char>(peek);
      std::string s(1, ch);
      advance();
      return Token(TokenType::UNKNOWN, s, line);
  }
}

Token DragonLexer::WS() {
  // TODO: finish me. The WS() method should consume all whitespace characters (spaces, tabs, newlines)
  std::string text;
  int startLine = line;
  
  while (std::isspace(peek) && peek != EOF_CHAR) {
    text += static_cast<char>(peek);
    advance();
  }
  return Token(TokenType::WS, text, startLine);
  //return Token(TokenType::WS, " ", ???);
}

Token DragonLexer::ID() {
  // TODO: finish me. The ID() method should consume an identifier or keyword.
  std::string text;
  int startLine = line;
  
  // 读取第一个字母
  text += static_cast<char>(peek);
  advance();
  
  // 读取后续的字母或数字
  while (std::isalnum(peek) && peek != EOF_CHAR) {
    text += static_cast<char>(peek);
    advance();
  }
  
  // 检查是否是关键字
  if (text == "if") {
    return Token(TokenType::IF, text, startLine);
  } else if (text == "else") {
    return Token(TokenType::ELSE, text, startLine);
  } else {
    return Token(TokenType::ID, text, startLine);
  }
}

Token DragonLexer::NUMBER() {
  // TODO: finish me. The NUMBER() method should consume an integer, real number, or scientific notation number.
  std::string text;
  int startLine = line;
  TokenType type = TokenType::INT; 
  
  // 读取整数部分
  bool lastWasQuote = false;
  bool hasDigits = false;
  
  while (peek != EOF_CHAR) {
    if (std::isdigit(peek)) {
      text += static_cast<char>(peek);
      lastWasQuote = false;
      hasDigits = true;
      advance();
    } else if (peek == '\'' && !lastWasQuote && hasDigits) {
      // 单引号只能在数字中间，不能在开头或连续出现
      text += static_cast<char>(peek);
      lastWasQuote = true;
      advance();
    } else {
      break;
    }
  }
  
  // 如果以单引号结尾，需要回退
  if (!text.empty() && text.back() == '\'') {
    text.pop_back();
    resetPos(getPosition() - 1);
  }
  
  // 检查是否有小数点
  if (peek == '.') {
    // 查看下一个字符是否是数字
    int savedPos = getPosition();
    std::string savedText = text;
    
    text += static_cast<char>(peek);
    advance();
    
    // 读取小数部分
    bool hasFractionDigits = false;
    lastWasQuote = false;
    
    while (peek != EOF_CHAR) {
      if (std::isdigit(peek)) {
        text += static_cast<char>(peek);
        lastWasQuote = false;
        hasFractionDigits = true;
        advance();
      } else if (peek == '\'' && !lastWasQuote && hasFractionDigits) {
        text += static_cast<char>(peek);
        lastWasQuote = true;
        advance();
      } else {
        break;
      }
    }
    
    // 如果小数部分没有数字，则回退到整数状态
    if (!hasFractionDigits) {
      resetPos(savedPos);
      text = savedText;
    } else {
      type = TokenType::REAL;
      
      // 如果以单引号结尾，回退
      if (!text.empty() && text.back() == '\'') {
        text.pop_back();
        resetPos(getPosition() - 1);
      }
    }
  }
  
  // 检查是否有科学计数法部分
  if ((peek == 'e' || peek == 'E') && hasDigits) {
    int savedPos = getPosition();
    std::string savedText = text;   
    text += static_cast<char>(peek);
    advance();    
    // 检查是否有符号
    if (peek == '+' || peek == '-') {
      text += static_cast<char>(peek);
      advance();
    }
    
    // 读取指数部分
    bool hasExponentDigits = false;
    lastWasQuote = false;
    while (peek != EOF_CHAR) {
      if (std::isdigit(peek)) {
        text += static_cast<char>(peek);
        hasExponentDigits = true;
        lastWasQuote = false;
        advance();
      } else if (peek == '\'' && !lastWasQuote && hasExponentDigits) {
        text += static_cast<char>(peek);
        lastWasQuote = true;
        advance();
      } else {
        break;
      }
    }
    
    // 如果指数部分没有数字，则回退
    if (!hasExponentDigits) {
      resetPos(savedPos);
      text = savedText;
    } else {
      type = TokenType::SCI;
      
      // 如果以单引号结尾，需要回退
      if (!text.empty() && text.back() == '\'') {
        text.pop_back();
        resetPos(getPosition() - 1);
      }
    }
  }
  
  return Token(type, text, startLine);
}
