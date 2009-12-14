#define INC_scope_D 1
/*
 * DECLARE FUNCTIONS AND GLOBALS
 */
#ifndef INC_builtin_D
#include "../ZUDIR/builtin.d.h"
#endif
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
#ifndef INC_symbol_D
#include "../ZUDIR/symbol.d.h"
#endif
#ifndef INC_tokenize_D
#include "../ZUDIR/tokenize.d.h"
#endif
#ifndef INC_usedfile_D
#include "../ZUDIR/usedfile.d.h"
#endif
#ifndef INC_zimbufile_D
#include "../ZUDIR/zimbufile.d.h"
#endif
#ifndef INC_libarg_D
#include "../lib/ZUDIR/libarg.d.h"
#endif
#ifndef INC_libio_D
#include "../lib/ZUDIR/libio.d.h"
#endif
#ifndef INC_libsys_D
#include "../lib/ZUDIR/libsys.d.h"
#endif
#ifndef INC_libthread_D
#include "../lib/ZUDIR/libthread.d.h"
#endif
Zbool CScope__FisClassScope(CScope *THIS);
CSymbol *CScope__FgetSymbol(CScope *THIS, char *Aname);
CSymbol *CScope__FgetSymbol__1(CScope *THIS, char *Aname, Zbool AclassCheck);
CSymbol *CScope__FgetSymbol__2(CScope *THIS, char *Aname, CNode *Anode);
CSymbol *CScope__FgetSymbol__3(CScope *THIS, char *Aname, Zenum Atype, Zbool AclassCheck, CNode *Anode);
CSymbol *CScope__FgetSymbolInt(CScope *THIS, char *Aname, Zenum Atype, Zbool AinShared, Zbool AclassCheck, CNode *Anode);
Zbool CScope__FusedAsZwt(CScope *THIS);
Zbool CScope__FusedAsZimbu(CScope *THIS);
CSymbol *CScope__FfindNodeSymbol(CScope *THIS, CNode *Anode);
CSymbol *CScope__FfindNodeSymbol__1(CScope *THIS, CNode *Anode, Zenum Atype, Zbool Aerror);
void CScope__FaddImport(CScope *THIS, CZimbuFile *Aimport);
Zenum CScope__FgetSymbolType(CScope *THIS, char *Aname);
Zenum CScope__FgetAType(CScope *THIS, char *Aname);
CSymbol *CScope__FaddSymbol(CScope *THIS, char *Aname, Zenum Atype, CNode *Anode, Zbool AdupOk);
void CScope__FaddMember(CScope *THIS, CSymbol *Asym);
CSymbol *CScope__FaddSymbol__1(CScope *THIS, char *Aname, CSymbol *Asp, CNode *Anode, Zbool AdupOk);
CSymbol *CScope__FfindMatchingFunc(CScope *THIS, char *Aname, CListHead *AargList, CSymbol *Askip, Zbool AsearchOuter, Zbool Aconvert);
void CScope__FlistMatchingFunc(CScope *THIS, char *Aname, CListHead *AargList, Zbool Aall);
Zenum CScope__FgetReturnType(CScope *THIS);
void CScope__FaddUsedItem(CScope *THIS, char *Aname);
void CScope__FmergeKeywords(CScope *THIS, CScope *Ascope);
CListHead *CScope__X__VpredefinedSymbols;
CSymbol *CScope__X__FfindPredefinedSymbol(char *Aname, Zenum Atype);
void CScope__X__FloadPredefinedSymbols();
void CScope__X__FaddPredefinedSymbol(CSymbol *Asym);
CScope *CScope__X__FnewScope(CScope *Aouter, Zbool AforwardDeclare);
void Iscope();
