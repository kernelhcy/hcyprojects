#define INC_node_S 1
/*
 * STRUCTS
 */
#ifndef INC_attr_S
#include "../ZUDIR/attr.s.h"
#endif
#ifndef INC_conversion_S
#include "../ZUDIR/conversion.s.h"
#endif
#ifndef INC_error_S
#include "../ZUDIR/error.s.h"
#endif
#ifndef INC_token_S
#include "../ZUDIR/token.s.h"
#endif
#ifndef INC_pos_S
#include "../ZUDIR/pos.s.h"
#endif
#ifndef INC_scope_S
#include "../ZUDIR/scope.s.h"
#endif
#ifndef INC_symbol_S
#include "../ZUDIR/symbol.s.h"
#endif
#ifndef INC_usedfile_S
#include "../ZUDIR/usedfile.s.h"
#endif
char *CNode__X__EType[] = {
"unknown",
"eof",
"nil",
"this",
"parent",
"id",
"ref",
"int",
"nat",
"string",
"char",
"bool",
"status",
"list",
"dict",
"dictPair",
"array",
"any",
"var",
"module",
"lib_module",
"class",
"shared",
"interface",
"object",
"i_object",
"bits_small",
"bits_big",
"bits_decl",
"enum",
"enum_value",
"enum_decl",
"equal_def",
"new_def",
"func_def",
"proc_def",
"func_ref",
"proc_ref",
"import",
"main",
"copydirect",
"cfunc",
"gen_if",
"gen_elseif",
"gen_else",
"if",
"else",
"elseif",
"while",
"for",
"in",
"break",
"continue",
"proceed",
"assign",
"minassign",
"plusassign",
"stringassign",
"block",
"new",
"copy",
"size",
"function",
"switch",
"case",
"default",
"declare",
"do",
"until",
"plusplus",
"minmin",
"return",
"exit",
"bit_and",
"bit_or",
"bit_xor",
"op_concat",
"op_mult",
"op_div",
"op_rem",
"op_plus",
"op_minus",
"op_negative",
"op_tilde",
"op_not",
"op_pre_incr",
"op_pre_decr",
"op_post_incr",
"op_post_decr",
"op_rshift",
"op_lshift",
"op_equal",
"op_notequal",
"op_gt",
"op_gte",
"op_lt",
"op_lte",
"op_is",
"op_isnot",
"op_isa",
"op_isnota",
"op_and",
"op_or",
"op_alt",
"parens",
"member",
"bits_value",
"method",
"subscript",
"typespec",
"op_comma",
"lib_method",
};
struct CNode__S {
  Zenum Vn_type;
  Zint Vn_int;
  char *Vn_string;
  Zint Vn_undefined;
  CNode *Vn_returnType;
  CNode *Vn_cond;
  CNode *Vn_left;
  CNode *Vn_right;
  CNode *Vn_next;
  Zenum Vn_conversion;
  CSymbol *Vn_ret_class;
  CPos *Vn_start;
  CPos *Vn_end;
  CSymbol *Vn_symbol;
  CSymbol *Vn_returnSymbol;
  Zbbits Vn_attr;
  char *Vn_scopeName;
  Zenum Vn_nodeType;
  CUsedFile *Vn_usedFile;
};
