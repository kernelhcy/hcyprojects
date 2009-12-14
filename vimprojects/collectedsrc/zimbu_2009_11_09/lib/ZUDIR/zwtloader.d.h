#define INC_zwtloader_D 1
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
Zbool MZWTloader__VuseServer;
CSymbol *MZWTloader__VmoduleSymbol;
CBuiltin *MZWTloader__Fprepare();
void MZWTloader__Fsetup(CBuiltin *Abuiltin);
void Izwtloader();
