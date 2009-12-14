#define INC_libthread_D 1
/*
 * DECLARE FUNCTIONS AND GLOBALS
 */
#ifndef INC_attr_D
#include "../../ZUDIR/attr.d.h"
#endif
#ifndef INC_config_D
#include "../../ZUDIR/config.d.h"
#endif
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
CSymbol *MLibTHREAD__VmoduleSym;
CSymbol *MLibTHREAD__VthreadSym;
Zbool MLibTHREAD__VuseThread;
CDictHead *MLibTHREAD__VthreadNewClasses;
CDictHead *MLibTHREAD__VthreadNewProcClasses;
CDictHead *MLibTHREAD__VthreadBodyClasses;
CDictHead *MLibTHREAD__VthreadStartClasses;
CDictHead *MLibTHREAD__VthreadSetprocClasses;
CDictHead *MLibTHREAD__VthreadWaitClasses;
CDictHead *MLibTHREAD__VthreadKillClasses;
CListHead *MLibTHREAD__FgetSymbols();
void MLibTHREAD__Fthreads(CSymbol *Asym, CSymbol *Aclass, CNode *Am_node, CSContext *Actx);
void MLibTHREAD__Fcurrent(CSymbol *Asym, CSymbol *Aclass, CNode *Am_node, CSContext *Actx);
void MLibTHREAD__FthreadNew(CSymbol *Asym, CSymbol *Aclass, CNode *Am_node, CSContext *Actx);
void MLibTHREAD__FthreadNewProc(CSymbol *Asym, CSymbol *Aclass, CNode *Am_node, CSContext *Actx);
void MLibTHREAD__FthreadBody(CSymbol *Asym, CSymbol *Aclass, CNode *Am_node, CSContext *Actx);
void MLibTHREAD__FthreadStart(CSymbol *Asym, CSymbol *Aclass, CNode *Am_node, CSContext *Actx);
void MLibTHREAD__FthreadSetProc(CSymbol *Asym, CSymbol *Aclass, CNode *Am_node, CSContext *Actx);
void MLibTHREAD__FthreadWait(CSymbol *Asym, CSymbol *Aclass, CNode *Am_node, CSContext *Actx);
void MLibTHREAD__FthreadKill(CSymbol *Asym, CSymbol *Aclass, CNode *Am_node, CSContext *Actx);
CSymbol *MLibTHREAD__FgenerateVarname(CNode *Anode, CSContext *Actx, CSymbol *AdestSym);
void MLibTHREAD__FthreadRemove(CSymbol *Asym, CSymbol *Aclass, CNode *Am_node, CSContext *Actx);
char *MLibTHREAD__FparentClassName(CSymbol *Asym, CDictHead *Aclasses);
void MLibTHREAD__FcheckUsed();
void MLibTHREAD__FwriteIncludes(CScope *AtopScope, FILE *Afd);
void MLibTHREAD__FwriteTypedefs(CScope *AtopScope, FILE *Afd);
void MLibTHREAD__FwriteDecl(CScope *AtopScope, FILE *Afd);
void MLibTHREAD__FwriteFuncLead(FILE *Afd, char *Aname, char *Afuncname);
void MLibTHREAD__FwriteBody(CScope *AtopScope, FILE *Afd);
void Ilibthread();
