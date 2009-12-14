#define INC_dictstuff_D 1
/*
 * DECLARE FUNCTIONS AND GLOBALS
 */
#ifndef INC_error_D
#include "../ZUDIR/error.d.h"
#endif
#ifndef INC_generate_D
#include "../ZUDIR/generate.d.h"
#endif
#ifndef INC_liststuff_D
#include "../ZUDIR/liststuff.d.h"
#endif
#ifndef INC_node_D
#include "../ZUDIR/node.d.h"
#endif
#ifndef INC_output_D
#include "../ZUDIR/output.d.h"
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
Zbool MDictStuff__VuseDict;
Zbool MDictStuff__VuseZDictHas;
Zbool MDictStuff__VuseZDictRemove;
Zbool MDictStuff__VuseZDictFind;
Zbool MDictStuff__VuseZDictGetPtr;
Zbool MDictStuff__VuseZDictGetPtrDef;
Zbool MDictStuff__VuseZDictGetInt;
Zbool MDictStuff__VuseZDictGetIntDef;
Zbool MDictStuff__VuseZDictToString;
Zbool MDictStuff__VuseZDictKeys;
Zbool MDictStuff__VuseZDictCopy;
Zbool MDictStuff__VuseZDictClear;
CSymbol *MDictStuff__FgenerateMethodCall(CSymbol *Asp, CNode *Am_node, CSContext *Actx, CSymbol *AdestSym);
CSymbol *MDictStuff__FgenerateVarname(CNode *Anode, CSContext *Actx, CSymbol *AdestSym);
CSymbol *MDictStuff__FgenExpr(CNode *Anode, CSContext *Actx, CSymbol *AdestSym);
CSymbol *MDictStuff__FgenerateSubscript(CSymbol *Asym, CNode *Anode, Zbool Alvalue, CSContext *Actx, CSymbol *AdestSym);
CSymbol *MDictStuff__FgenerateGet(CSymbol *Asym, CNode *Adict_node, CNode *Aarg_node, CNode *Adef_node, CSContext *Actx, CSymbol *AdestSym);
void MDictStuff__FgenKeyArgs(CSymbol *Asym, CNode *Anode, CSContext *Actx);
void MDictStuff__FgenerateDictPart_C(CNode *Anode, CSymbol *Aret, CSContext *Actx);
void MDictStuff__FwriteTypedefs(CScope *AtopScope, FILE *Afd);
void MDictStuff__FwriteDecl(CScope *AtopScope, FILE *Afd);
void MDictStuff__FwriteBody(CScope *AtopScope, FILE *Afd);
void Idictstuff();
