#define INC_scontext_S 1
/*
 * STRUCTS
 */
#ifndef INC_output_S
#include "../ZUDIR/output.s.h"
#endif
#ifndef INC_resolve_S
#include "../ZUDIR/resolve.s.h"
#endif
#ifndef INC_scope_S
#include "../ZUDIR/scope.s.h"
#endif
struct CSContext__S {
  CScope *Vscope;
  Zoref *Vgen;
  COutput *Vout;
};
