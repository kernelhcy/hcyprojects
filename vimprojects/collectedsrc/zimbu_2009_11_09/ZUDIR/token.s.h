#define INC_token_S 1
/*
 * STRUCTS
 */
#ifndef INC_error_S
#include "../ZUDIR/error.s.h"
#endif
#ifndef INC_pos_S
#include "../ZUDIR/pos.s.h"
#endif
char *CToken__EType[] = {
"unknown",
"eof",
"sep",
"line_sep",
"comment",
"empty",
"nil",
"this",
"parent",
"false",
"true",
"ok",
"fail",
"string",
"rawstring",
"char",
"any",
"var",
"id",
"import",
"main",
"copy_start",
"c",
"js",
"module",
"class",
"shared",
"interface",
"mixin",
"enum",
"bits",
"func",
"proc",
"extends",
"implements",
"define",
"replace",
"return",
"exit",
"new",
"equal",
"size",
"copy",
"i",
"gen_if",
"gen_elseif",
"gen_else",
"if",
"elseif",
"else",
"while",
"break",
"continue",
"proceed",
"switch",
"case",
"default",
"do",
"until",
"for",
"in",
"comma",
"semicolon",
"colon",
"question",
"dot",
"p_open",
"p_close",
"c_open",
"c_close",
"sq_open",
"sq_close",
"minus",
"minmin",
"plus",
"plusplus",
"tilde",
"star",
"slash",
"percent",
"concat",
"bit_or",
"bit_xor",
"amp",
"rshift",
"lshift",
"assign",
"minassign",
"plusassign",
"multassign",
"divassign",
"percentassign",
"tildeassign",
"andassign",
"orassign",
"xorassign",
"stringassign",
"lshiftassign",
"rshiftassign",
"isis",
"notequal",
"and",
"or",
"not",
"gt",
"gte",
"lt",
"lte",
"is",
"isnot",
"isa",
"isnota",
};
struct CToken__S {
  Zenum Vtype;
  char *Vvalue;
  CPos *VstartPos;
  CPos *VendPos;
};