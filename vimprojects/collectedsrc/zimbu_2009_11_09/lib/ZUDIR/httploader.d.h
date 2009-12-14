#define INC_httploader_D 1
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
#ifndef INC_write_c_D
#include "../../ZUDIR/write_c.d.h"
#endif
Zbool MHTTPloader__VuseServer;
CSymbol *MHTTPloader__VmoduleSymbol;
CBuiltin *MHTTPloader__Fprepare();
void MHTTPloader__Fsetup(CBuiltin *Abuiltin);
void MHTTPloader__FwriteIncl(CScope *AtopScope, FILE *Afd);
void Ihttploader();
