#pragma once

#include <iostream>

#include "ptx_parser_core.h"

namespace PARSER {
ParseResult parse(std::vector<LEXER::Token> const& tokens) {
  return Parser(tokens).parse();
}

ParseResult parse(std::vector<LEXER::Token>&& tokens) {
  return Parser(std::move(tokens)).parse();
}

ParseResult parse_file(std::string const& path) {
  auto contents = LEXER::read_file(path);
  if (!contents) {
    ParseResult result;
    result.errors.push_back({"failed to open PTX file: " + path, 0, 0});
    return result;
  }
  return parse(LEXER::lex(*contents).tokens);
}

void print_parse_summary(ParseResult const& result) {
  std::cout << "version: " << result.file.version_major << '.'
            << result.file.version_minor << '\n';
  std::cout << "target: " << result.file.target << '\n';
  std::cout << "address_size: " << result.file.address_size << '\n';
  std::cout << "kernels: " << result.file.kernels.size() << '\n';
  for (auto const& kernel : result.file.kernels) {
    std::cout << "- " << kernel.name << " params=" << kernel.parameters.size()
              << " instructions=" << kernel.instructions.size() << '\n';
  }
  if (!result.errors.empty()) {
    std::cout << "errors: " << result.errors.size() << '\n';
    for (auto const& error : result.errors) {
      std::cout << error.line << ':' << error.column << "  " << error.message
                << '\n';
    }
  }
}
}  // namespace PARSER
