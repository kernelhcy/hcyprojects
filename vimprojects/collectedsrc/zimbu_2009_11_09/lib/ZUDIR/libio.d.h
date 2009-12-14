#define INC_libio_D 1
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
CSymbol *MLibIO__VfileSym;
CSymbol *MLibIO__FgetSymbol();
void MLibIO__FreadByte(CSymbol *Asym, CSymbol *Aclass, CNode *Aarg_node, CSContext *Actx);
void MLibIO__FreadFile(CSymbol *Asym, CSymbol *Aclass, CNode *Aarg_node, CSContext *Actx);
void MLibIO__Fwrite(CSymbol *Asym, CSymbol *Aclass, CNode *Aarg_node, CSContext *Actx);
void MLibIO__FwriteLine(CSymbol *Asym, CSymbol *Aclass, CNode *Aarg_node, CSContext *Actx);
void MLibIO__FwriteByte(CSymbol *Asym, CSymbol *Aclass, CNode *Aarg_node, CSContext *Actx);
void MLibIO__Fflush(CSymbol *Asym, CSymbol *Aclass, CNode *Aarg_node, CSContext *Actx);
void MLibIO__FfileReadByte(CSymbol *Asym, CSymbol *Aclass, CNode *Am_node, CSContext *Actx);
void MLibIO__FfileWrite(CSymbol *Asym, CSymbol *Aclass, CNode *Am_node, CSContext *Actx);
void MLibIO__FfileWriteByte(CSymbol *Asym, CSymbol *Aclass, CNode *Am_node, CSContext *Actx);
void MLibIO__FfileClose(CSymbol *Asym, CSymbol *Aclass, CNode *Am_node, CSContext *Actx);
void MLibIO__FfileReader(CSymbol *Asym, CSymbol *Aclass, CNode *Aarg_node, CSContext *Actx);
void MLibIO__FfileWriter(CSymbol *Asym, CSymbol *Aclass, CNode *Aarg_node, CSContext *Actx);
void MLibIO__Frename(CSymbol *Asym, CSymbol *Aclass, CNode *Aarg_node, CSContext *Actx);
void MLibIO__Fdelete(CSymbol *Asym, CSymbol *Aclass, CNode *Aarg_node, CSContext *Actx);
void MLibIO__Fmkdir(CSymbol *Asym, CSymbol *Aclass, CNode *Aarg_node, CSContext *Actx);
void MLibIO__Fstat(CSymbol *Asym, CSymbol *Aclass, CNode *Aarg_node, CSContext *Actx);
CSymbol *MLibIO__FgenerateVarname(CNode *Anode, CSContext *Actx, CSymbol *AdestSym);
CSymbol *MLibIO__FgenExpr(CNode *Anode, CSContext *Actx, CSymbol *AdestSym);
Zbool MLibIO__VuseZStat;
void MLibIO__FwriteInclude(CScope *AtopScope, FILE *Afd);
void MLibIO__FwriteTypedef(CScope *AtopScope, FILE *Afd);
void MLibIO__FwriteDecl(CScope *AtopScope, FILE *Afd);
void MLibIO__FwriteBody(CScope *AtopScope, FILE *Afd);
void Ilibio();
