#define INC_libsys_D 1
/*
 * DECLARE FUNCTIONS AND GLOBALS
 */
#ifndef INC_generate_D
#include "../../ZUDIR/generate.d.h"
#endif
#ifndef INC_node_D
#include "../../ZUDIR/node.d.h"
#endif
#ifndef INC_output_D
#include "../../ZUDIR/output.d.h"
#endif
#ifndef INC_scontext_D
#include "../../ZUDIR/scontext.d.h"
#endif
#ifndef INC_scope_D
#include "../../ZUDIR/scope.d.h"
#endif
#ifndef INC_symbol_D
#include "../../ZUDIR/symbol.d.h"
#endif
#ifndef INC_write_c_D
#include "../../ZUDIR/write_c.d.h"
#endif
Zbool MLibSYS__VuseZsleep;
CSymbol *MLibSYS__FgetSymbol();
void MLibSYS__Fshell(CSymbol *Asym, CSymbol *Aclass, CNode *Aarg_node, CSContext *Actx);
void MLibSYS__Fsleep(CSymbol *Asym, CSymbol *Aclass, CNode *Aarg_node, CSContext *Actx);
void MLibSYS__FsleepSec(CSymbol *Asym, CSymbol *Aclass, CNode *Aarg_node, CSContext *Actx);
void MLibSYS__Fmalloc(CSymbol *Asym, CSymbol *Aclass, CNode *Aarg_node, CSContext *Actx);
void MLibSYS__Frealloc(CSymbol *Asym, CSymbol *Aclass, CNode *Aarg_node, CSContext *Actx);
CSymbol *MLibSYS__FgenExpr(CNode *Anode, CSContext *Actx, CSymbol *AdestSym);
void MLibSYS__FwriteBody(CScope *AtopScope, FILE *Afd);
void Ilibsys();
