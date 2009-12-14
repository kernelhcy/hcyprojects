#define INC_libarg_D 1
/*
 * DECLARE FUNCTIONS AND GLOBALS
 */
#ifndef INC_generate_D
#include "../../ZUDIR/generate.d.h"
#endif
#ifndef INC_liststuff_D
#include "../../ZUDIR/liststuff.d.h"
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
Zbool MLibARG__VuseCount;
Zbool MLibARG__VuseName;
Zbool MLibARG__VuseArgs;
Zbool MLibARG__VuseZArgGet;
Zbool MLibARG__VuseZArgAll;
Zbool MLibARG__VuseZArgClean;
CSymbol *MLibARG__VmoduleSymbol;
CSymbol *MLibARG__FgetSymbol();
void MLibARG__Fsize(CSymbol *Asym, CSymbol *Aclass, CNode *Aarg_node, CSContext *Actx);
void MLibARG__Fget(CSymbol *Asym, CSymbol *Aclass, CNode *Aarg_node, CSContext *Actx);
void MLibARG__FgetAll(CSymbol *Asym, CSymbol *Aclass, CNode *Aarg_node, CSContext *Actx);
void MLibARG__FgetClean(CSymbol *Asym, CSymbol *Aclss, CNode *Aarg_node, CSContext *Actx);
void MLibARG__FcheckUsed();
void MLibARG__FwriteDecl(CScope *AtopScope, FILE *Afd);
void MLibARG__FwriteBodies(CScope *AtopScope, FILE *Afd);
void MLibARG__FwriteMain(COutput *Aout);
void Ilibarg();
