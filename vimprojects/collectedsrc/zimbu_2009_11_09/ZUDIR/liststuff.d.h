#define INC_liststuff_D 1
/*
 * DECLARE FUNCTIONS AND GLOBALS
 */
#ifndef INC_error_D
#include "../ZUDIR/error.d.h"
#endif
#ifndef INC_generate_D
#include "../ZUDIR/generate.d.h"
#endif
#ifndef INC_node_D
#include "../ZUDIR/node.d.h"
#endif
#ifndef INC_output_D
#include "../ZUDIR/output.d.h"
#endif
#ifndef INC_resolve_D
#include "../ZUDIR/resolve.d.h"
#endif
#ifndef INC_scontext_D
#include "../ZUDIR/scontext.d.h"
#endif
#ifndef INC_scope_D
#include "../ZUDIR/scope.d.h"
#endif
#ifndef INC_symbol_D
#include "../ZUDIR/symbol.d.h"
#endif
CSymbol *MListStuff__FgenerateMethodCall(CSymbol *Asp, CNode *Am_node, CSContext *Actx, CSymbol *AdestSym);
CSymbol *MListStuff__FgenerateSubscript(CSymbol *Asym, CNode *Anode, Zbool Alvalue, CSContext *Actx, CSymbol *AdestSym);
CSymbol *MListStuff__FgenerateVarname(CNode *Anode, CSContext *Actx, CSymbol *AdestSym);
CSymbol *MListStuff__FgenExpr(CNode *Anode, CSContext *Actx, CSymbol *AdestSym);
CSymbol *MListStuff__FgenerateListPart_C(CNode *Anode, CSContext *Actx, CSymbol *AdestSym);
void MListStuff__FwriteTypedefs_C(CScope *AtopScope, FILE *Afd);
void MListStuff__FwriteDecl_C(CScope *AtopScope, FILE *Afd);
void MListStuff__FwriteBody_C(CScope *AtopScope, FILE *Afd);
CSymbol *MListStuff__FgenerateListPart_JS(CNode *Anode, CSContext *Actx, CSymbol *AdestSym);
CSymbol *MListStuff__FoneListPart_JS(CNode *Anode, CSContext *Actx, CSymbol *AdestSym);
void MListStuff__FwriteBody_JS(CScope *AtopScope, FILE *Afd);
void Iliststuff();
