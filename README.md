### Lexer
+ [x] Define TokenType enum (DIRECTIVE, TYPE_QUALIFIER, OPCODE, REGISTER, SPECIAL_REGISTER, LABEL_DEF, IDENTIFIER, INTEGER, FLOAT, STRING, punctuation, AT, BANG, ERROR, END_OF_FILE)
+ [x] Define Token struct (type, lexeme, line, column, optional integerValue, optional floatValue)
+ [x] Define LexerError struct (message, line, column)
+ [x] Define LexResult struct (tokens vector, errors vector)
+ [x] Implement token_type_name() debug helper
+ [x] Implement character classification helpers (is_hex_digit, is_identifier_start, is_identifier_body, is_percent_body)
+ [x] Build static directive keyword table
+ [x] Build static type qualifier keyword table
+ [x] Build static opcode keyword table
+ [x] Build static special register keyword table
+ [x] Implement skip_whitespace_and_comments() (handles //, /* */, newline tracking)
+ [x] Implement scan_dot_token() (directives and type qualifiers, table lookup to classify)
+ [x] Implement scan_percent_token() (virtual registers and special registers, dot+in+name support)
+ [x] Implement scan_identifier_or_label() (colon lookahead for LABEL_DEF, opcode table lookup)
+ [x] Implement scan_number_or_float() (hex 0x, PTX hex float 0f/0d, decimal, standard float)
+ [x] Implement scan_string_literal() (escape handling, unterminated error)
+ [x] Implement scan_single_char() for all punctuation tokens
+ [x] Implement emit_token() / emit_error_token() / begin_token() mechanics
+ [x] Implement main lex() loop
+ [x] Implement read_file() utility
+ [x] Implement print_tokens() debug printer
+ [x] Implement print_tokens_from_file() entry point

### Parser
+ [x] Define Opcode enum (UNKNOWN + all PTX opcodes)
+ [x] Define SpecialRegister enum (UNKNOWN + all special registers)
+ [x] Define ParseError struct
+ [x] Define Parameter struct (name, type string, optional alignment)
+ [x] Define SharedDeclaration struct (name, type, optional alignment, optional element_count)
+ [x] Define all operand structs: IntegerOperand, FloatOperand, RegisterOperand, SpecialRegisterOperand, AddressOperand, VectorOperand, LabelReferenceOperand
+ [x] Define Operand as std::variant of all operand types
+ [x] Define Instruction struct (label, predicate, opcode, qualifiers, operands, branch_target)
+ [x] Define PTXKernel struct (name, visible, is_entry, parameters, register_types, shared_declarations, raw_declarations, instructions, label_to_index)
+ [x] Define PTXFile struct (version, target, address_size, raw_global_declarations, kernels)
+ [x] Define ParseResult struct
+ [x] Implement opcode_from_lexeme() lookup table
+ [x] Implement special_register_from_lexeme() lookup table
+ [x] Implement trim_label_suffix() helper
+ [x] Implement parser primitives: peek() with offset, consume(), match(), check(), check_directive(), expect(), expect_directive()
+ [x] Implement parse_file_header() (.version, .target, .address_size)
+ [x] Implement parse_kernel() (visibility, .entry/.func, name, params, optional body)
+ [x] Implement parse_parameter_list() (.param, optional .align, type, name)
+ [x] Implement parse_kernel_body() (declaration vs instruction dispatch loop)
+ [x] Implement .reg declaration parsing with array expansion (%r<N> → %r0..%r(N+1) in register_types map)
+ [x] Implement .shared declaration parsing (optional .align, type, name, optional [count])
+ [x] Implement parse_instruction() (label, predicate, opcode, qualifier chain, operands, semicolon)
+ [x] Implement parse_operand() (dispatches on token type to all six operand forms)
+ [x] Implement parse_address_operand() ([base] and [base+offset] / [base+offset])
+ [x] Implement parse_vector_operand() ({%r0, %r1, ...})
+ [x] Implement negative immediate operand handling (MINUS + INTEGER)
+ [x] Implement resolve_labels() (label_to_index map, unresolved reference errors)
+ [x] Implement four error recovery methods: synchronize_to_top_level, synchronize_to_kernel_boundary, synchronize_instruction, synchronize_parameter
+ [x] Implement parse_float_value() (hex+encoded 0f/0d via memcpy bit+cast, standard stod)
+ [x] Implement parse_file() entry point (reads file, lexes, parses)
+ [x] Implement print_parse_summary() debug printer

### IR
+ [ ] Add bool is_param_relative{false} to AddressOperand
  + [ ] Set it to true in parse_address_operand() when base token is IDENTIFIER (not REGISTER)
+ [ ] Add size_t index{0} to Instruction
  + [ ] Set instruction.index = kernel.instructions.size() before each push_back in parse_kernel_body()
+ [ ] Add bool is_block_leader{false} to Instruction
+ [ ] Add bool is_block_terminator{false} to Instruction
+ [ ] Add std::optional<size_t> basic_block_id to Instruction
+ [ ] Define BasicBlock struct in PARSER namespace (before PTXKernel)
  + [ ] Fields: id, first_instruction, last_instruction
  + [ ] Fields: successors (vector of block IDs), predecessors (vector of block IDs)
  + [ ] Fields: immediate_dominator (optional block ID), dominated_blocks (vector of block IDs)
  + [ ] Fields: is_loop_header{false}, loop_depth{0}
+ [ ] Add std::vector<BasicBlock> basic_blocks to PTXKernel
+ [ ] Add std::optional<size_t> entry_block_id to PTXKernel
+ [ ] Define PTXBaseType enum (UNKNOWN, U8–U64, S8–S64, F16/F32/F64, B8–B64, PRED)
+ [ ] Define MemorySpace enum (UNKNOWN, GLOBAL, SHARED, LOCAL, PARAM, CONST, REG, GENERIC)
+ [ ] Define QualifierInfo struct (data_type, memory_space, is_vector, vector_width, has_rounding, saturate, is_approx, flush_to_zero)
+ [ ] Implement base_type_from_qualifier(string) — .f32 → F32 etc.
+ [ ] Implement memory_space_from_qualifier(string) — .global → GLOBAL etc.
+ [ ] Implement parse_qualifiers(vector<string>) → QualifierInfo
+ [ ] Implement get_register_type(PTXKernel, reg_name) → PTXBaseType
+ [ ] Implement special_register_type(SpecialRegister) → PTXBaseType (static, no context needed)
+ [ ] Implement operand_type(PTXKernel, Operand) → PTXBaseType (variant dispatch)
+ [ ] Implement is_branch(Opcode) → bool (BRA, BRX)
+ [ ] Implement is_terminator(Opcode) → bool (BRA, BRX, RET, EXIT)
+ [ ] Implement is_conditional_branch(Instruction) → bool (is branch + has_predicate)
+ [ ] Write IR validation function
  + [ ] Every RegisterOperand in every instruction resolves in register_types
  + [ ] Every instruction index equals its position in the vector
  + [ ] Every label_to_index value is within bounds of instructions vector
