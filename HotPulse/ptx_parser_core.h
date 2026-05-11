#pragma once

#include <cstring>
#include <iostream>
#include <utility>

#include "ptx_parser_utils.h"

namespace PARSER {
class Parser {
 public:
  explicit Parser(std::vector<LEXER::Token> tokens)
      : _tokens(std::move(tokens)) {}

  ParseResult parse() {
    parse_file_header();
    while (!is_at_end()) {
      if (check_directive(".visible") || check_directive(".entry") ||
          check_directive(".func")) {
        auto kernel = parse_kernel();
        if (kernel.has_value()) {
          _result.file.kernels.push_back(std::move(*kernel));
        }
        continue;
      }

      if (check_directive(".global") || check_directive(".const")) {
        _result.file.raw_global_declarations.push_back(
            consume_until_semicolon_text());
        continue;
      }

      if (check_directive(".file") || check_directive(".loc")) {
        skip_current_line();
        continue;
      }

      if (check_directive(".pragma")) {
        consume();
        skip_until_semicolon();
        continue;
      }

      if (match(LEXER::TokenType::END_OF_FILE)) {
        break;
      }

      log_error(peek(), "unexpected top-level token");
      synchronize_to_top_level();
    }

    return std::move(_result);
  }

 private:
  std::vector<LEXER::Token> _tokens;
  std::size_t _pos{0};
  ParseResult _result;

  bool is_at_end() const {
    return _pos >= _tokens.size() ||
           _tokens[_pos].type == LEXER::TokenType::END_OF_FILE;
  }

  const LEXER::Token& peek(std::size_t offset = 0) const {
    std::size_t index = _pos + offset;
    if (index >= _tokens.size()) {
      return _tokens.back();
    }
    return _tokens[index];
  }

  const LEXER::Token& consume() {
    if (!is_at_end()) {
      return _tokens[_pos++];
    }
    return _tokens.back();
  }

  bool match(LEXER::TokenType type) {
    if (!is_at_end() && peek().type == type) {
      ++_pos;
      return true;
    }
    return false;
  }

  bool check_directive(std::string const& lexeme) const {
    return !is_at_end() && peek().lexeme == lexeme;
  }

  bool check(LEXER::TokenType type) const {
    return !is_at_end() && peek().type == type;
  }

  void log_error(LEXER::Token const& token, std::string const& message) {
    _result.errors.push_back({message, token.line, token.column});
  }

  bool expect(LEXER::TokenType type, std::string const& message) {
    if (check(type)) {
      consume();
      return true;
    }
    log_error(peek(), message);
    return false;
  }

  bool expect_directive(std::string const& lexeme, std::string const& message) {
    if (check_directive(lexeme)) {
      consume();
      return true;
    }
    log_error(peek(), message);
    return false;
  }

  void parse_file_header() {
    if (!expect_directive(".version", "expected .version at start of file")) {
      synchronize_to_top_level();
      return;
    }

    if (check(LEXER::TokenType::FLOAT)) {
      std::pair<int, int> version = parse_version_pair(peek().lexeme);
      _result.file.version_major = version.first;
      _result.file.version_minor = version.second;
      consume();
    } else {
      if (expect(LEXER::TokenType::INTEGER, "expected major version integer")) {
        _result.file.version_major =
            static_cast<int>(parse_integer_or_zero(_tokens[_pos - 1].lexeme));
      }
      if (expect(LEXER::TokenType::INTEGER, "expected minor version integer")) {
        _result.file.version_minor =
            static_cast<int>(parse_integer_or_zero(_tokens[_pos - 1].lexeme));
      }
    }

    if (!expect_directive(".target", "expected .target after .version")) {
      return;
    }
    if (!expect(LEXER::TokenType::IDENTIFIER,
                "expected architecture target identifier")) {
      return;
    }
    _result.file.target = _tokens[_pos - 1].lexeme;

    if (check_directive(".address_size")) {
      consume();
      if (expect(LEXER::TokenType::INTEGER,
                 "expected integer after .address_size")) {
        _result.file.address_size =
            static_cast<int>(parse_integer_or_zero(_tokens[_pos - 1].lexeme));
      }
    }
  }

  std::optional<PTXKernel> parse_kernel() {
    PTXKernel kernel;
    kernel.visible = false;
    kernel.is_entry = false;

    if (check_directive(".visible")) {
      consume();
      kernel.visible = true;
    }

    if (check_directive(".entry")) {
      consume();
      kernel.is_entry = true;
    } else if (check_directive(".func")) {
      consume();
      kernel.is_entry = false;
    } else {
      log_error(peek(), "expected .entry or .func");
      return std::nullopt;
    }

    if (!expect(LEXER::TokenType::IDENTIFIER, "expected kernel name")) {
      synchronize_to_kernel_boundary();
      return std::nullopt;
    }
    kernel.name = _tokens[_pos - 1].lexeme;

    if (!expect(LEXER::TokenType::LPAREN, "expected '(' after kernel name")) {
      synchronize_to_kernel_boundary();
      return std::nullopt;
    }

    parse_parameter_list(kernel);

    if (!expect(LEXER::TokenType::RPAREN,
                "expected ')' after parameter list")) {
      synchronize_to_kernel_boundary();
      return std::nullopt;
    }

    if (check(LEXER::TokenType::LBRACE)) {
      consume();
      parse_kernel_body(kernel);
    } else if (check(LEXER::TokenType::SEMICOLON)) {
      consume();
    } else {
      log_error(peek(), "expected '{' or ';' after kernel signature");
      synchronize_to_kernel_boundary();
      return std::nullopt;
    }

    resolve_labels(kernel);
    return kernel;
  }

  void parse_parameter_list(PTXKernel& kernel) {
    while (!is_at_end() && !check(LEXER::TokenType::RPAREN)) {
      if (check(LEXER::TokenType::COMMA)) {
        consume();
        continue;
      }

      Parameter parameter;
      if (!expect_directive(".param", "expected .param in parameter list")) {
        synchronize_parameter();
        continue;
      }

      if (check_directive(".align")) {
        consume();
        if (expect(LEXER::TokenType::INTEGER,
                   "expected alignment integer after .align")) {
          parameter.alignment =
              static_cast<int>(parse_integer_or_zero(_tokens[_pos - 1].lexeme));
        }
      }

      if (!expect(LEXER::TokenType::TYPE_QUALIFIER,
                  "expected type qualifier in parameter")) {
        synchronize_parameter();
        continue;
      }
      parameter.type = _tokens[_pos - 1].lexeme;

      if (!expect(LEXER::TokenType::IDENTIFIER, "expected parameter name")) {
        synchronize_parameter();
        continue;
      }
      parameter.name = _tokens[_pos - 1].lexeme;

      kernel.parameters.push_back(std::move(parameter));
    }
  }

  void parse_kernel_body(PTXKernel& kernel) {
    while (!is_at_end() && !check(LEXER::TokenType::RBRACE)) {
      if (check_directive(".reg")) {
        parse_register_declaration(kernel);
        continue;
      }
      if (check_directive(".shared")) {
        parse_shared_declaration(kernel);
        continue;
      }
      if (check_directive(".local") || check_directive(".param") ||
          check_directive(".const")) {
        kernel.raw_declarations.push_back(consume_until_semicolon_text());
        continue;
      }
      if (check_directive(".file") || check_directive(".loc")) {
        skip_current_line();
        continue;
      }
      if (check_directive(".pragma")) {
        consume();
        skip_until_semicolon();
        continue;
      }
      if (check(LEXER::TokenType::LABEL_DEF) || check(LEXER::TokenType::AT) ||
          check(LEXER::TokenType::OPCODE)) {
        Instruction instruction = parse_instruction();
        kernel.instructions.push_back(std::move(instruction));
        continue;
      }

      log_error(peek(), "unexpected token in kernel body");
      synchronize_instruction();
    }

    if (check(LEXER::TokenType::RBRACE)) {
      consume();
    } else {
      log_error(peek(), "missing closing '}' for kernel body");
    }
  }

  void parse_register_declaration(PTXKernel& kernel) {
    consume();
    if (!expect(LEXER::TokenType::TYPE_QUALIFIER,
                "expected register type after .reg")) {
      skip_until_semicolon();
      return;
    }
    std::string type = _tokens[_pos - 1].lexeme;

    if (!expect(LEXER::TokenType::REGISTER,
                "expected register name after .reg type")) {
      skip_until_semicolon();
      return;
    }
    std::string base_name = _tokens[_pos - 1].lexeme;

    if (check(LEXER::TokenType::LANGLE)) {
      consume();
      if (!expect(LEXER::TokenType::INTEGER,
                  "expected register count inside < >")) {
        skip_until_semicolon();
        return;
      }
      std::int64_t count = parse_integer_or_zero(_tokens[_pos - 1].lexeme);
      if (!expect(LEXER::TokenType::RANGLE,
                  "expected '>' after register count")) {
        skip_until_semicolon();
        return;
      }
      for (std::int64_t i = 0; i < count; ++i) {
        kernel.register_types[base_name + std::to_string(i)] = type;
      }
    } else {
      kernel.register_types[base_name] = type;
    }

    expect(LEXER::TokenType::SEMICOLON, "expected ';' after .reg declaration");
  }

  void parse_shared_declaration(PTXKernel& kernel) {
    consume();
    SharedDeclaration decl;
    if (check_directive(".align")) {
      consume();
      if (expect(LEXER::TokenType::INTEGER,
                 "expected alignment after .align")) {
        decl.alignment =
            static_cast<int>(parse_integer_or_zero(_tokens[_pos - 1].lexeme));
      }
    }

    if (!expect(LEXER::TokenType::TYPE_QUALIFIER,
                "expected type qualifier after .shared")) {
      skip_until_semicolon();
      return;
    }
    decl.type = _tokens[_pos - 1].lexeme;

    if (!expect(LEXER::TokenType::IDENTIFIER, "expected shared memory name")) {
      skip_until_semicolon();
      return;
    }
    decl.name = _tokens[_pos - 1].lexeme;

    if (check(LEXER::TokenType::LBRACKET)) {
      consume();
      if (expect(LEXER::TokenType::INTEGER,
                 "expected element count inside []")) {
        decl.element_count = parse_integer_or_zero(_tokens[_pos - 1].lexeme);
      }
      if (!expect(LEXER::TokenType::RBRACKET,
                  "expected ']' after element count")) {
        skip_until_semicolon();
        return;
      }
    }

    kernel.shared_declarations.push_back(std::move(decl));
    expect(LEXER::TokenType::SEMICOLON,
           "expected ';' after .shared declaration");
  }

  Instruction parse_instruction() {
    Instruction instruction;

    if (check(LEXER::TokenType::LABEL_DEF)) {
      instruction.has_label = true;
      instruction.label = trim_label_suffix(consume().lexeme);
      if (!check(LEXER::TokenType::AT) && !check(LEXER::TokenType::OPCODE)) {
        return instruction;
      }
    }

    if (check(LEXER::TokenType::AT)) {
      consume();
      instruction.has_predicate = true;
      if (check(LEXER::TokenType::BANG)) {
        consume();
        instruction.predicate_negated = true;
      }
      if (expect(LEXER::TokenType::REGISTER,
                 "expected predicate register after @")) {
        instruction.predicate_register = _tokens[_pos - 1].lexeme;
      }
    }

    if (!expect(LEXER::TokenType::OPCODE, "expected opcode")) {
      synchronize_instruction();
      return instruction;
    }
    instruction.opcode_lexeme = _tokens[_pos - 1].lexeme;
    instruction.opcode = opcode_from_lexeme(instruction.opcode_lexeme);

    while (check(LEXER::TokenType::TYPE_QUALIFIER)) {
      instruction.qualifiers.push_back(consume().lexeme);
    }

    if (instruction.opcode == Opcode::RET ||
        instruction.opcode == Opcode::EXIT) {
      expect(LEXER::TokenType::SEMICOLON,
             "expected ';' after terminator instruction");
      return instruction;
    }

    if (instruction.opcode == Opcode::BRA ||
        instruction.opcode == Opcode::BRX ||
        instruction.opcode == Opcode::CALL) {
      if (check(LEXER::TokenType::IDENTIFIER)) {
        instruction.branch_target = consume().lexeme;
        instruction.operands.push_back(
            LabelReferenceOperand{instruction.branch_target});
      } else {
        log_error(peek(), "expected branch target label");
      }
      expect(LEXER::TokenType::SEMICOLON,
             "expected ';' after branch instruction");
      return instruction;
    }

    while (!is_at_end() && !check(LEXER::TokenType::SEMICOLON)) {
      if (check(LEXER::TokenType::COMMA)) {
        consume();
        continue;
      }

      instruction.operands.push_back(parse_operand());

      if (!is_at_end() && check(LEXER::TokenType::PIPE)) {
        consume();
        instruction.operands.push_back(parse_operand());
      }
    }

    expect(LEXER::TokenType::SEMICOLON, "expected ';' after instruction");
    return instruction;
  }

  Operand parse_operand() {
    if (check(LEXER::TokenType::REGISTER)) {
      return RegisterOperand{consume().lexeme};
    }
    if (check(LEXER::TokenType::SPECIAL_REGISTER)) {
      SpecialRegisterOperand operand;
      operand.lexeme = consume().lexeme;
      operand.reg = special_register_from_lexeme(operand.lexeme);
      return operand;
    }
    if (check(LEXER::TokenType::INTEGER)) {
      IntegerOperand operand;
      operand.lexeme = consume().lexeme;
      operand.is_hex = starts_with(operand.lexeme, "0x") ||
                       starts_with(operand.lexeme, "0X");
      operand.value = parse_signed_integer(operand.lexeme);
      return operand;
    }
    if (check(LEXER::TokenType::FLOAT)) {
      FloatOperand operand;
      operand.lexeme = consume().lexeme;
      operand.is_hex_encoded = starts_with(operand.lexeme, "0f") ||
                               starts_with(operand.lexeme, "0d");
      operand.value = parse_float_value(operand.lexeme);
      return operand;
    }
    if (check(LEXER::TokenType::LBRACKET)) {
      return parse_address_operand();
    }
    if (check(LEXER::TokenType::LBRACE)) {
      return parse_vector_operand();
    }
    if (check(LEXER::TokenType::IDENTIFIER)) {
      return LabelReferenceOperand{consume().lexeme};
    }
    if (check(LEXER::TokenType::STRING)) {
      return LabelReferenceOperand{consume().lexeme};
    }
    if (check(LEXER::TokenType::DIRECTIVE) ||
        check(LEXER::TokenType::TYPE_QUALIFIER)) {
      return LabelReferenceOperand{consume().lexeme};
    }
    if (check(LEXER::TokenType::MINUS) &&
        peek(1).type == LEXER::TokenType::INTEGER) {
      consume();
      IntegerOperand operand;
      operand.lexeme = consume().lexeme;
      operand.is_hex = starts_with(operand.lexeme, "0x") ||
                       starts_with(operand.lexeme, "0X");
      operand.value = -parse_signed_integer(operand.lexeme);
      return operand;
    }

    log_error(peek(), "unexpected operand");
    consume();
    return IntegerOperand{};
  }

  Operand parse_address_operand() {
    consume();
    AddressOperand operand;
    if (check(LEXER::TokenType::REGISTER) ||
        check(LEXER::TokenType::IDENTIFIER)) {
      operand.base_register = consume().lexeme;
    } else {
      log_error(peek(), "expected base register in address expression");
    }
    if (check(LEXER::TokenType::PLUS) || check(LEXER::TokenType::MINUS)) {
      bool negative = match(LEXER::TokenType::MINUS);
      if (!negative) {
        consume();
      }
      if (expect(LEXER::TokenType::INTEGER,
                 "expected integer offset in address expression")) {
        operand.offset = parse_signed_integer(_tokens[_pos - 1].lexeme);
        if (negative) {
          operand.offset = -operand.offset;
        }
      }
    }
    expect(LEXER::TokenType::RBRACKET,
           "expected ']' to close address expression");
    return operand;
  }

  Operand parse_vector_operand() {
    consume();
    VectorOperand operand;
    while (!is_at_end() && !check(LEXER::TokenType::RBRACE)) {
      if (check(LEXER::TokenType::COMMA)) {
        consume();
        continue;
      }
      if (check(LEXER::TokenType::REGISTER)) {
        operand.registers.push_back(consume().lexeme);
        continue;
      }
      log_error(peek(), "expected register in vector operand");
      consume();
    }
    expect(LEXER::TokenType::RBRACE, "expected '}' after vector operand");
    return operand;
  }

  void resolve_labels(PTXKernel& kernel) {
    for (std::size_t i = 0; i < kernel.instructions.size(); ++i) {
      if (kernel.instructions[i].has_label &&
          !kernel.instructions[i].label.empty()) {
        kernel.label_to_index[kernel.instructions[i].label] = i;
      }
    }

    for (Instruction& instruction : kernel.instructions) {
      if (instruction.opcode != Opcode::BRA &&
          instruction.opcode != Opcode::BRX &&
          instruction.opcode != Opcode::CALL) {
        continue;
      }
      if (instruction.branch_target.empty()) {
        continue;
      }
      if (kernel.label_to_index.find(instruction.branch_target) ==
          kernel.label_to_index.end()) {
        _result.errors.push_back(
            {"unresolved label reference: " + instruction.branch_target, 0, 0});
      }
    }
  }

  void synchronize_to_top_level() {
    while (!is_at_end()) {
      if (check_directive(".visible") || check_directive(".entry") ||
          check_directive(".func") || check_directive(".global") ||
          check_directive(".const") || check_directive(".file") ||
          check_directive(".loc") || check_directive(".pragma") ||
          check(LEXER::TokenType::END_OF_FILE)) {
        return;
      }
      consume();
    }
  }

  void synchronize_to_kernel_boundary() {
    while (!is_at_end() && !check(LEXER::TokenType::END_OF_FILE)) {
      if (check_directive(".visible") || check_directive(".entry") ||
          check_directive(".func")) {
        return;
      }
      consume();
    }
  }

  void synchronize_instruction() {
    while (!is_at_end() && !check(LEXER::TokenType::SEMICOLON) &&
           !check(LEXER::TokenType::RBRACE)) {
      consume();
    }
    if (check(LEXER::TokenType::SEMICOLON) || check(LEXER::TokenType::RBRACE)) {
      consume();
    }
  }

  void synchronize_parameter() {
    while (!is_at_end() && !check(LEXER::TokenType::COMMA) &&
           !check(LEXER::TokenType::RPAREN)) {
      consume();
    }
  }

  void skip_until_semicolon() {
    while (!is_at_end() && !check(LEXER::TokenType::SEMICOLON) &&
           !check(LEXER::TokenType::RBRACE)) {
      consume();
    }
    if (check(LEXER::TokenType::SEMICOLON)) {
      consume();
    }
  }

  void skip_current_line() {
    if (is_at_end()) {
      return;
    }
    int line = peek().line;
    while (!is_at_end() && peek().line == line) {
      consume();
    }
  }

  std::string consume_until_semicolon_text() {
    std::string text;
    while (!is_at_end() && !check(LEXER::TokenType::SEMICOLON) &&
           !check(LEXER::TokenType::RBRACE)) {
      if (!text.empty()) text.push_back(' ');
      text += consume().lexeme;
    }
    if (check(LEXER::TokenType::SEMICOLON)) consume();
    return text;
  }

  static bool starts_with(std::string const& s, char const* prefix) {
    std::string p(prefix);
    return s.rfind(p, 0) == 0;
  }

  static std::int64_t parse_integer_or_zero(std::string const& lexeme) {
    try {
      return std::stoll(lexeme, nullptr, 0);
    } catch (...) {
      return 0;
    }
  }

  static std::pair<int, int> parse_version_pair(std::string const& lexeme) {
    try {
      std::size_t dot = lexeme.find('.');
      if (dot == std::string::npos) {
        return {static_cast<int>(std::stoll(lexeme, nullptr, 10)), 0};
      }
      int major =
          static_cast<int>(std::stoll(lexeme.substr(0, dot), nullptr, 10));
      int minor =
          static_cast<int>(std::stoll(lexeme.substr(dot + 1), nullptr, 10));
      return {major, minor};
    } catch (...) {
      return {0, 0};
    }
  }

  static std::int64_t parse_signed_integer(std::string const& lexeme) {
    try {
      return std::stoll(lexeme, nullptr, 0);
    } catch (...) {
      return 0;
    }
  }

  static double parse_float_value(std::string const& lexeme) {
    try {
      if (lexeme.size() == 10 &&
          (lexeme.rfind("0f", 0) == 0 || lexeme.rfind("0F", 0) == 0)) {
        std::uint32_t bits = static_cast<std::uint32_t>(
            std::stoul(lexeme.substr(2), nullptr, 16));
        double out = 0.0;
        float f = 0.0f;
        std::memcpy(&f, &bits, sizeof(bits));
        out = static_cast<double>(f);
        return out;
      }
      if (lexeme.size() == 18 &&
          (lexeme.rfind("0d", 0) == 0 || lexeme.rfind("0D", 0) == 0)) {
        std::uint64_t bits = std::stoull(lexeme.substr(2), nullptr, 16);
        double out = 0.0;
        std::memcpy(&out, &bits, sizeof(bits));
        return out;
      }
      return std::stod(lexeme);
    } catch (...) {
      return 0.0;
    }
  }
};
}  // namespace PARSER
