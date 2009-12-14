#define INC_resolve_S 1
/*
 * STRUCTS
 */
#ifndef INC_conversion_S
#include "../ZUDIR/conversion.s.h"
#endif
#ifndef INC_dictstuff_S
#include "../ZUDIR/dictstuff.s.h"
#endif
#ifndef INC_error_S
#include "../ZUDIR/error.s.h"
#endif
#ifndef INC_generate_S
#include "../ZUDIR/generate.s.h"
#endif
#ifndef INC_liststuff_S
#include "../ZUDIR/liststuff.s.h"
#endif
#ifndef INC_node_S
#include "../ZUDIR/node.s.h"
#endif
#ifndef INC_output_S
#include "../ZUDIR/output.s.h"
#endif
#ifndef INC_scontext_S
#include "../ZUDIR/scontext.s.h"
#endif
#ifndef INC_scope_S
#include "../ZUDIR/scope.s.h"
#endif
#ifndef INC_symbol_S
#include "../ZUDIR/symbol.s.h"
#endif
#ifndef INC_zimbufile_S
#include "../ZUDIR/zimbufile.s.h"
#endif
#ifndef INC_write_c_S
#include "../ZUDIR/write_c.s.h"
#endif
#ifndef INC_write_js_S
#include "../ZUDIR/write_js.s.h"
#endif
char *CResolve__X__ETargetLang[] = {
"none",
"c",
"js",
};
struct CResolve__S {
  Zenum VtargetLang;
  char *VpermuName;
};
