#pragma once

#include <unordered_map>

#include "ptx_parser_types.h"

namespace PARSER {
bool is_type_qualifier_token(LEXER::Token const& token) {
  return token.type == LEXER::TokenType::TYPE_QUALIFIER;
}

Opcode opcode_from_lexeme(std::string const& lexeme) {
  static const std::unordered_map<std::string, Opcode> table = {
      {"ld", Opcode::LD},
      {"st", Opcode::ST},
      {"mov", Opcode::MOV},
      {"add", Opcode::ADD},
      {"sub", Opcode::SUB},
      {"mul", Opcode::MUL},
      {"mad", Opcode::MAD},
      {"div", Opcode::DIV},
      {"rem", Opcode::REM},
      {"abs", Opcode::ABS},
      {"neg", Opcode::NEG},
      {"min", Opcode::MIN},
      {"max", Opcode::MAX},
      {"cvt", Opcode::CVT},
      {"cvta", Opcode::CVTA},
      {"setp", Opcode::SETP},
      {"selp", Opcode::SELP},
      {"slct", Opcode::SLCT},
      {"bra", Opcode::BRA},
      {"brx", Opcode::BRX},
      {"call", Opcode::CALL},
      {"ret", Opcode::RET},
      {"exit", Opcode::EXIT},
      {"bar", Opcode::BAR},
      {"membar", Opcode::MEMBAR},
      {"atom", Opcode::ATOM},
      {"red", Opcode::RED},
      {"shl", Opcode::SHL},
      {"shr", Opcode::SHR},
      {"and", Opcode::AND},
      {"or", Opcode::OR},
      {"xor", Opcode::XOR},
      {"not", Opcode::NOT},
      {"cnot", Opcode::CNOT},
      {"popc", Opcode::POPC},
      {"clz", Opcode::CLZ},
      {"bfind", Opcode::BFIND},
      {"fma", Opcode::FMA},
      {"rcp", Opcode::RCP},
      {"sqrt", Opcode::SQRT},
      {"rsqrt", Opcode::RSQRT},
      {"sin", Opcode::SIN},
      {"cos", Opcode::COS},
      {"lg2", Opcode::LG2},
      {"ex2", Opcode::EX2},
      {"testp", Opcode::TESTP},
      {"copysign", Opcode::COPYSIGN},
      {"shfl", Opcode::SHFL}};

  auto it = table.find(lexeme);
  return it == table.end() ? Opcode::UNKNOWN : it->second;
}

SpecialRegister special_register_from_lexeme(std::string const& lexeme) {
  static const std::unordered_map<std::string, SpecialRegister> table = {
      {"%tid.x", SpecialRegister::TID_X},
      {"%tid.y", SpecialRegister::TID_Y},
      {"%tid.z", SpecialRegister::TID_Z},
      {"%ntid.x", SpecialRegister::NTID_X},
      {"%ntid.y", SpecialRegister::NTID_Y},
      {"%ntid.z", SpecialRegister::NTID_Z},
      {"%ctaid.x", SpecialRegister::CTAID_X},
      {"%ctaid.y", SpecialRegister::CTAID_Y},
      {"%ctaid.z", SpecialRegister::CTAID_Z},
      {"%nctaid.x", SpecialRegister::NCTAID_X},
      {"%nctaid.y", SpecialRegister::NCTAID_Y},
      {"%nctaid.z", SpecialRegister::NCTAID_Z},
      {"%laneid", SpecialRegister::LANEID},
      {"%warpid", SpecialRegister::WARPID},
      {"%nwarpid", SpecialRegister::NWARPID},
      {"%smid", SpecialRegister::SMID},
      {"%nsmid", SpecialRegister::NSMID},
      {"%gridid", SpecialRegister::GRIDID},
      {"%clock", SpecialRegister::CLOCK},
      {"%clock64", SpecialRegister::CLOCK64},
      {"%pm0", SpecialRegister::PM0},
      {"%pm1", SpecialRegister::PM1},
      {"%pm2", SpecialRegister::PM2},
      {"%pm3", SpecialRegister::PM3},
      {"%pm4", SpecialRegister::PM4},
      {"%pm5", SpecialRegister::PM5},
      {"%pm6", SpecialRegister::PM6},
      {"%pm7", SpecialRegister::PM7}};

  auto it = table.find(lexeme);
  return it == table.end() ? SpecialRegister::UNKNOWN : it->second;
}

std::string trim_label_suffix(std::string lexeme) {
  if (!lexeme.empty() && lexeme.back() == ':') {
    lexeme.pop_back();
  }
  return lexeme;
}
}  // namespace PARSER
