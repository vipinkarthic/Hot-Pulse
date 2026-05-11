#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

#include "ptx_lexer.h"

namespace PARSER {
enum class Opcode {
  UNKNOWN,
  LD,
  ST,
  MOV,
  ADD,
  SUB,
  MUL,
  MAD,
  DIV,
  REM,
  ABS,
  NEG,
  MIN,
  MAX,
  CVT,
  CVTA,
  SETP,
  SELP,
  SLCT,
  BRA,
  BRX,
  CALL,
  RET,
  EXIT,
  BAR,
  MEMBAR,
  ATOM,
  RED,
  SHL,
  SHR,
  AND,
  OR,
  XOR,
  NOT,
  CNOT,
  POPC,
  CLZ,
  BFIND,
  FMA,
  RCP,
  SQRT,
  RSQRT,
  SIN,
  COS,
  LG2,
  EX2,
  TESTP,
  COPYSIGN,
  SHFL
};

enum class SpecialRegister {
  UNKNOWN,
  TID_X,
  TID_Y,
  TID_Z,
  NTID_X,
  NTID_Y,
  NTID_Z,
  CTAID_X,
  CTAID_Y,
  CTAID_Z,
  NCTAID_X,
  NCTAID_Y,
  NCTAID_Z,
  LANEID,
  WARPID,
  NWARPID,
  SMID,
  NSMID,
  GRIDID,
  CLOCK,
  CLOCK64,
  PM0,
  PM1,
  PM2,
  PM3,
  PM4,
  PM5,
  PM6,
  PM7
};

struct ParseError {
  std::string message;
  int line{1};
  int column{1};
};

struct Parameter {
  std::string name;
  std::string type;
  std::optional<int> alignment;
};

struct SharedDeclaration {
  std::string name;
  std::string type;
  std::optional<int> alignment;
  std::optional<std::int64_t> element_count;
};

struct IntegerOperand {
  std::int64_t value{};
  bool is_hex{false};
  std::string lexeme;
};

struct FloatOperand {
  double value{};
  bool is_hex_encoded{false};
  std::string lexeme;
};

struct RegisterOperand {
  std::string name;
};

struct SpecialRegisterOperand {
  SpecialRegister reg{SpecialRegister::UNKNOWN};
  std::string lexeme;
};

struct AddressOperand {
  std::string base_register;
  std::int64_t offset{0};
};

struct VectorOperand {
  std::vector<std::string> registers;
};

struct LabelReferenceOperand {
  std::string label;
};

using Operand = std::variant<RegisterOperand, SpecialRegisterOperand,
                             IntegerOperand, FloatOperand, AddressOperand,
                             VectorOperand, LabelReferenceOperand>;

struct Instruction {
  std::string label;
  bool has_label{false};
  bool predicate_negated{false};
  bool has_predicate{false};
  std::string predicate_register;
  Opcode opcode{Opcode::UNKNOWN};
  std::string opcode_lexeme;
  std::vector<std::string> qualifiers;
  std::vector<Operand> operands;
  std::string branch_target;
};

struct PTXKernel {
  std::string name;
  bool visible{false};
  bool is_entry{true};
  std::vector<Parameter> parameters;
  std::unordered_map<std::string, std::string> register_types;
  std::vector<SharedDeclaration> shared_declarations;
  std::vector<std::string> raw_declarations;
  std::vector<Instruction> instructions;
  std::unordered_map<std::string, std::size_t> label_to_index;
};

struct PTXFile {
  int version_major{0};
  int version_minor{0};
  std::string target;
  int address_size{0};
  std::vector<std::string> raw_global_declarations;
  std::vector<PTXKernel> kernels;
};

struct ParseResult {
  PTXFile file;
  std::vector<ParseError> errors;
};
}  // namespace PARSER
