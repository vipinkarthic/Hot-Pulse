#pragma once

#include <cctype>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <optional>
#include <sstream>
#include <string>
#include <unordered_set>
#include <vector>

namespace LEXER {
enum class TokenType {
  DIRECTIVE,
  TYPE_QUALIFIER,
  OPCODE,
  REGISTER,
  SPECIAL_REGISTER,
  LABEL_DEF,
  IDENTIFIER,
  INTEGER,
  FLOAT,
  STRING,
  LBRACE,
  RBRACE,
  LPAREN,
  RPAREN,
  LBRACKET,
  RBRACKET,
  LANGLE,
  RANGLE,
  COMMA,
  SEMICOLON,
  PIPE,
  PLUS,
  MINUS,
  STAR,
  SLASH,
  AT,
  BANG,
  COLON,
  EQUAL,
  ERROR,
  END_OF_FILE,
};

struct Token {
  TokenType type{};
  std::string lexeme;
  int line{1};
  int column{1};
  std::optional<std::uint64_t> integerValue;
  std::optional<double> floatValue;
};

struct LexerError {
  std::string message;
  int line{1};
  int column{1};
};

struct LexResult {
  std::vector<Token> tokens;
  std::vector<LexerError> errors;
};

std::string token_type_name(TokenType type) {
  switch (type) {
    case TokenType::DIRECTIVE:
      return "DIRECTIVE";
    case TokenType::TYPE_QUALIFIER:
      return "TYPE_QUALIFIER";
    case TokenType::OPCODE:
      return "OPCODE";
    case TokenType::REGISTER:
      return "REGISTER";
    case TokenType::SPECIAL_REGISTER:
      return "SPECIAL_REGISTER";
    case TokenType::LABEL_DEF:
      return "LABEL_DEF";
    case TokenType::IDENTIFIER:
      return "IDENTIFIER";
    case TokenType::INTEGER:
      return "INTEGER";
    case TokenType::FLOAT:
      return "FLOAT";
    case TokenType::STRING:
      return "STRING";
    case TokenType::LBRACE:
      return "LBRACE";
    case TokenType::RBRACE:
      return "RBRACE";
    case TokenType::LPAREN:
      return "LPAREN";
    case TokenType::RPAREN:
      return "RPAREN";
    case TokenType::LBRACKET:
      return "LBRACKET";
    case TokenType::RBRACKET:
      return "RBRACKET";
    case TokenType::LANGLE:
      return "LANGLE";
    case TokenType::RANGLE:
      return "RANGLE";
    case TokenType::COMMA:
      return "COMMA";
    case TokenType::SEMICOLON:
      return "SEMICOLON";
    case TokenType::PLUS:
      return "PLUS";
    case TokenType::MINUS:
      return "MINUS";
    case TokenType::STAR:
      return "STAR";
    case TokenType::SLASH:
      return "SLASH";
    case TokenType::AT:
      return "AT";
    case TokenType::BANG:
      return "BANG";
    case TokenType::COLON:
      return "COLON";
    case TokenType::EQUAL:
      return "EQUAL";
    case TokenType::ERROR:
      return "ERROR";
    case TokenType::END_OF_FILE:
      return "END_OF_FILE";
  }
  return "UNKNOWN";
}

bool is_hex_digit(char c) {
  return std::isxdigit(static_cast<unsigned char>(c)) != 0;
}

bool is_identifier_start(char c) {
  return std::isalpha(static_cast<unsigned char>(c)) != 0 || c == '_' ||
         c == '$';
}

bool is_identifier_body(char c) {
  return std::isalnum(static_cast<unsigned char>(c)) != 0 || c == '_' ||
         c == '$';
}

bool is_percent_body(char c) { return is_identifier_body(c) || c == '.'; }

class Lexer {
 public:
  explicit Lexer(std::string source) : _source(std::move(source)) {}

  LexResult lex() {
    while (true) {
      skip_whitespace_and_comments();

      if (is_at_end()) {
        begin_token();
        emit_token(TokenType::END_OF_FILE);
        break;
      }

      char c = peek();
      if (c == '.') {
        scan_dot_token();
      } else if (c == '%') {
        scan_percent_token();
      } else if (is_identifier_start(c)) {
        scan_identifier_or_label();
      } else if (std::isdigit(static_cast<unsigned char>(c)) != 0) {
        scan_number_or_float();
      } else if (c == '"') {
        scan_string_literal();
      } else {
        switch (c) {
          case '{':
            scan_single_char(TokenType::LBRACE);
            break;
          case '}':
            scan_single_char(TokenType::RBRACE);
            break;
          case '(':
            scan_single_char(TokenType::LPAREN);
            break;
          case ')':
            scan_single_char(TokenType::RPAREN);
            break;
          case '[':
            scan_single_char(TokenType::LBRACKET);
            break;
          case ']':
            scan_single_char(TokenType::RBRACKET);
            break;
          case '<':
            scan_single_char(TokenType::LANGLE);
            break;
          case '>':
            scan_single_char(TokenType::RANGLE);
            break;
          case ',':
            scan_single_char(TokenType::COMMA);
            break;
          case ';':
            scan_single_char(TokenType::SEMICOLON);
            break;
          case '+':
            scan_single_char(TokenType::PLUS);
            break;
          case '-':
            scan_single_char(TokenType::MINUS);
            break;
          case '*':
            scan_single_char(TokenType::STAR);
            break;
          case '/':
            scan_single_char(TokenType::SLASH);
            break;
          case '@':
            scan_single_char(TokenType::AT);
            break;
          case '!':
            scan_single_char(TokenType::BANG);
            break;
          case ':':
            scan_single_char(TokenType::COLON);
            break;
          case '=':
            scan_single_char(TokenType::EQUAL);
            break;
          case '|':
            scan_single_char(TokenType::PIPE);
            break;
          default:
            begin_token();
            advance();
            emit_error_token("unexpected character");
            break;
        }
      }
    }

    return {std::move(_tokens), std::move(_errors)};
  }

 private:
  std::string _source;
  std::size_t _pos = 0;
  int _line = 1;
  int _column = 1;
  std::size_t _token_start_pos = 0;
  int _token_start_line = 1;
  int _token_start_column = 1;
  std::vector<Token> _tokens;
  std::vector<LexerError> _errors;

  static std::unordered_set<std::string> const& directive_table() {
    static std::unordered_set<std::string> const table = {
        ".version",      ".target",  ".address_size", ".visible", ".entry",
        ".func",         ".reg",     ".param",        ".local",   ".shared",
        ".global",       ".const",   ".align",        ".maxnreg", ".maxntid",
        ".minnctapersm", ".reqntid", ".pragma",       ".file",    ".loc"};
    return table;
  }

  static std::unordered_set<std::string> const& type_qualifier_table() {
    static std::unordered_set<std::string> const table = {
        ".f32",    ".f64",   ".u32",   ".u64",   ".s32", ".s64",  ".b8",
        ".b16",    ".b32",   ".b64",   ".pred",  ".v2",  ".v4",   ".global",
        ".shared", ".local", ".param", ".const", ".lo",  ".hi",   ".wide",
        ".rn",     ".rz",    ".rm",    ".rp",    ".sat", ".sync", ".approx",
        ".ftz",    ".eq",    ".ne",    ".lt",    ".le",  ".gt",   ".ge",
        ".to",     ".uni",   ".nc"};
    return table;
  }

  static std::unordered_set<std::string> const& opcode_table() {
    static std::unordered_set<std::string> const table = {
        "ld",     "st",   "mov",  "add", "sub",   "mul",   "mad",      "div",
        "rem",    "abs",  "neg",  "min", "max",   "cvt",   "cvta",     "setp",
        "selp",   "slct", "bra",  "brx", "call",  "ret",   "exit",     "bar",
        "membar", "atom", "red",  "shl", "shr",   "and",   "or",       "xor",
        "not",    "cnot", "popc", "clz", "bfind", "fma",   "rcp",      "sqrt",
        "rsqrt",  "sin",  "cos",  "lg2", "ex2",   "testp", "copysign", "shfl"};
    return table;
  }

  static std::unordered_set<std::string> const& special_register_table() {
    static std::unordered_set<std::string> const table = {
        "%tid.x",    "%tid.y",    "%tid.z",   "%ntid.x",  "%ntid.y",
        "%ntid.z",   "%ctaid.x",  "%ctaid.y", "%ctaid.z", "%nctaid.x",
        "%nctaid.y", "%nctaid.z", "%laneid",  "%warpid",  "%nwarpid",
        "%smid",     "%nsmid",    "%gridid",  "%clock",   "%clock64",
        "%pm0",      "%pm1",      "%pm2",     "%pm3",     "%pm4",
        "%pm5",      "%pm6",      "%pm7"};
    return table;
  }

  bool is_at_end() const { return _pos >= _source.size(); }

  char peek() const { return is_at_end() ? '\0' : _source[_pos]; }

  char peek_next() const {
    return (_pos + 1 >= _source.size()) ? '\0' : _source[_pos + 1];
  }

  char peek_next_next() const {
    return (_pos + 2 >= _source.size()) ? '\0' : _source[_pos + 2];
  }

  char advance() {
    if (is_at_end()) {
      return '\0';
    }

    char c = _source[_pos++];
    if (c == '\n') {
      ++_line;
      _column = 1;
    } else {
      ++_column;
    }
    return c;
  }

  void begin_token() {
    _token_start_pos = _pos;
    _token_start_line = _line;
    _token_start_column = _column;
  }

  void emit_token(TokenType type) {
    Token token;
    token.type = type;
    token.line = _token_start_line;
    token.column = _token_start_column;
    token.lexeme = _source.substr(_token_start_pos, _pos - _token_start_pos);
    _tokens.push_back(std::move(token));
  }

  void emit_token(TokenType type, std::string lexeme) {
    Token token;
    token.type = type;
    token.line = _token_start_line;
    token.column = _token_start_column;
    token.lexeme = std::move(lexeme);
    _tokens.push_back(std::move(token));
  }

  void emit_error_token(std::string message) {
    Token token;
    token.type = TokenType::ERROR;
    token.line = _token_start_line;
    token.column = _token_start_column;
    token.lexeme = _source.substr(_token_start_pos, _pos - _token_start_pos);
    _tokens.push_back(std::move(token));
    _errors.push_back(
        {std::move(message), _token_start_line, _token_start_column});
  }

  void add_error(std::string message, int line, int column) {
    _errors.push_back({std::move(message), line, column});
  }

  void skip_whitespace_and_comments() {
    bool consumed = true;
    while (consumed && !is_at_end()) {
      consumed = false;

      while (!is_at_end()) {
        char c = peek();
        if (c == ' ' || c == '\t' || c == '\r' || c == '\v' || c == '\f') {
          advance();
          consumed = true;
        } else if (c == '\n') {
          advance();
          consumed = true;
        } else {
          break;
        }
      }

      if (is_at_end()) {
        return;
      }

      if (peek() == '/' && peek_next() == '/') {
        while (!is_at_end() && peek() != '\n') {
          advance();
        }
        consumed = true;
        continue;
      }

      if (peek() == '/' && peek_next() == '*') {
        int comment_line = _line;
        int comment_column = _column;
        advance();
        advance();
        bool closed = false;
        while (!is_at_end()) {
          if (peek() == '*' && peek_next() == '/') {
            advance();
            advance();
            closed = true;
            break;
          }
          advance();
        }
        if (!closed) {
          add_error("unterminated block comment", comment_line, comment_column);
        }
        consumed = true;
      }
    }
  }

  void scan_single_char(TokenType type) {
    begin_token();
    advance();
    emit_token(type);
  }

  void scan_dot_token() {
    begin_token();
    advance();

    if (!std::isalpha(static_cast<unsigned char>(peek()))) {
      emit_error_token("expected letter after dot");
      return;
    }

    while (is_identifier_body(peek())) {
      advance();
    }

    std::string text =
        _source.substr(_token_start_pos, _pos - _token_start_pos);
    if (directive_table().count(text) != 0) {
      emit_token(TokenType::DIRECTIVE, std::move(text));
    } else {
      emit_token(TokenType::TYPE_QUALIFIER, std::move(text));
    }
  }

  void scan_percent_token() {
    begin_token();
    advance();

    if (!is_percent_body(peek())) {
      emit_error_token("expected register name after percent sign");
      return;
    }

    while (is_percent_body(peek())) {
      advance();
    }

    std::string text =
        _source.substr(_token_start_pos, _pos - _token_start_pos);
    if (special_register_table().count(text) != 0) {
      emit_token(TokenType::SPECIAL_REGISTER, std::move(text));
    } else {
      emit_token(TokenType::REGISTER, std::move(text));
    }
  }

  void scan_identifier_or_label() {
    begin_token();
    advance();
    while (is_identifier_body(peek())) {
      advance();
    }

    bool is_label = false;
    if (peek() == ':') {
      is_label = true;
      advance();
    }

    std::string text =
        _source.substr(_token_start_pos, _pos - _token_start_pos);
    if (is_label) {
      emit_token(TokenType::LABEL_DEF, std::move(text));
      return;
    }

    if (opcode_table().count(text) != 0) {
      emit_token(TokenType::OPCODE, std::move(text));
    } else {
      emit_token(TokenType::IDENTIFIER, std::move(text));
    }
  }

  void scan_number_or_float() {
    begin_token();

    if (peek() == '0' && (peek_next() == 'x' || peek_next() == 'X')) {
      advance();
      advance();
      while (is_hex_digit(peek())) {
        advance();
      }
      emit_token(TokenType::INTEGER);
      return;
    }

    if (peek() == '0' && (peek_next() == 'f' || peek_next() == 'd') &&
        is_hex_digit(peek_next_next())) {
      char kind = peek_next();
      std::size_t expected_digits = (kind == 'f') ? 8u : 16u;
      advance();
      advance();

      std::size_t digit_count = 0;
      while (digit_count < expected_digits && is_hex_digit(peek())) {
        advance();
        ++digit_count;
      }

      if (digit_count == expected_digits) {
        emit_token(TokenType::FLOAT);
        return;
      }

      while (is_hex_digit(peek())) {
        advance();
      }
      emit_error_token("invalid hex float literal");
      return;
    }

    bool is_float = false;
    while (std::isdigit(static_cast<unsigned char>(peek())) != 0) {
      advance();
    }

    if (peek() == '.') {
      is_float = true;
      advance();
      while (std::isdigit(static_cast<unsigned char>(peek())) != 0) {
        advance();
      }
    }

    if (peek() == 'e' || peek() == 'E') {
      char next = peek_next();
      char next_next = peek_next_next();
      if (std::isdigit(static_cast<unsigned char>(next)) != 0 ||
          ((next == '+' || next == '-') &&
           std::isdigit(static_cast<unsigned char>(next_next)) != 0)) {
        is_float = true;
        advance();
        if (peek() == '+' || peek() == '-') {
          advance();
        }
        while (std::isdigit(static_cast<unsigned char>(peek())) != 0) {
          advance();
        }
      }
    }

    if (is_float) {
      emit_token(TokenType::FLOAT);
    } else {
      Token token;
      token.type = TokenType::INTEGER;
      token.line = _token_start_line;
      token.column = _token_start_column;
      token.lexeme = _source.substr(_token_start_pos, _pos - _token_start_pos);
      try {
        token.integerValue = std::stoull(token.lexeme, nullptr, 0);
      } catch (...) {
      }
      _tokens.push_back(std::move(token));
    }
  }

  void scan_string_literal() {
    begin_token();
    advance();

    bool escaped = false;
    while (!is_at_end()) {
      char c = peek();
      if (escaped) {
        escaped = false;
        advance();
        continue;
      }
      if (c == '\\') {
        escaped = true;
        advance();
        continue;
      }
      if (c == '"') {
        advance();
        emit_token(TokenType::STRING);
        return;
      }
      if (c == '\n') {
        break;
      }
      advance();
    }

    emit_error_token("unterminated string literal");
  }
};

LexResult lex(std::string source) { return Lexer(std::move(source)).lex(); }

std::optional<std::string> read_file(std::string const& path) {
  std::ifstream input(path, std::ios::binary);
  if (!input.is_open()) {
    return std::nullopt;
  }

  std::ostringstream buffer;
  buffer << input.rdbuf();
  return buffer.str();
}

void print_tokens(LexResult const& result) {
  for (Token const& token : result.tokens) {
    std::cout << token.line << ':' << token.column << "  "
              << token_type_name(token.type) << "  "
              << std::quoted(token.lexeme) << '\n';
  }

  if (!result.errors.empty()) {
    std::cout << "\nLexer errors:\n";
    for (LexerError const& error : result.errors) {
      std::cout << error.line << ':' << error.column << "  " << error.message
                << '\n';
    }
  }
}

int print_tokens_from_file(std::string const& path) {
  std::optional<std::string> contents = read_file(path);
  if (!contents) {
    std::cerr << "Failed to open PTX file: " << path << '\n';
    return 1;
  }

  LexResult result = lex(*contents);
  print_tokens(result);
  return result.errors.empty() ? 0 : 2;
}

}  // namespace LEXER
