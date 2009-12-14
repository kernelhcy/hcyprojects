#define INC_infoloader_D 1
/*
 * DECLARE FUNCTIONS AND GLOBALS
 */
#ifndef INC_builtin_D
#include "../../ZUDIR/builtin.d.h"
#endif
#ifndef INC_node_D
#include "../../ZUDIR/node.d.h"
#endif
#ifndef INC_scope_D
#include "../../ZUDIR/scope.d.h"
#endif
#ifndef INC_symbol_D
#include "../../ZUDIR/symbol.d.h"
#endif
CSymbol *MINFOloader__VmoduleSymbol;
CBuiltin *MINFOloader__Fprepare();
void MINFOloader__Fsetup(CBuiltin *Abuiltin);
void Iinfoloader();
