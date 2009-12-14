#define INC_symbol_D 1
/*
 * DECLARE FUNCTIONS AND GLOBALS
 */
#ifndef INC_attr_D
#include "../ZUDIR/attr.d.h"
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
#ifndef INC_pos_D
#include "../ZUDIR/pos.d.h"
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
CSymbol *CSymbol__FNEW(Zenum A_type);
char *CSymbol__FtoString(CSymbol *THIS);
char *CSymbol__FtoString__1(CSymbol *THIS, char *Aindent, Zbool Arecurse);
char *CSymbol__FdumpSymbols(CSymbol *THIS);
void CSymbol__FaddFromSymbol(CSymbol *THIS, char *Adump);
CSymbol *CSymbol__Fcopy(CSymbol *THIS);
CSymbol *CSymbol__FcopyObject(CSymbol *THIS);
CSymbol *CSymbol__FaddMember(CSymbol *THIS, char *A_name, CSymbol *AtypeSym, Zint A_value);
CSymbol *CSymbol__FaddMember__1(CSymbol *THIS, CSymbol *AtypeSym);
void CSymbol__FremoveMember(CSymbol *THIS, CSymbol *Asym);
Zbool CSymbol__FhasMember(CSymbol *THIS);
void CSymbol__FaddChild(CSymbol *THIS, CSymbol *Achild);
void CSymbol__FaddInterface(CSymbol *THIS, CSymbol *Aitf, CNode *A_node);
Zint CSymbol__FfindChild(CSymbol *THIS, CSymbol *Achild);
Zint CSymbol__FchildIndex(CSymbol *THIS, CSymbol *Achild);
CSymbol *CSymbol__FaddLibMethod(CSymbol *THIS, char *A_name, void *AproduceProc, CSymbol *A_returnSymbol);
CListHead *CSymbol__FgetMemberList(CSymbol *THIS);
CSymbol *CSymbol__FfindMember(CSymbol *THIS, char *A_name);
CSymbol *CSymbol__FfindMatchingMember(CSymbol *THIS, CSymbol *Amember);
CSymbol *CSymbol__FfindMember__1(CSymbol *THIS, char *A_name, Zenum AtypeReq);
char *CSymbol__FgetClassName(CSymbol *THIS);
Zbool CSymbol__Fmatches(CSymbol *THIS, char *A_name, Zenum A_type);
CSymbol *CSymbol__FfindMatchingFunction(CSymbol *THIS, char *Amethod, CListHead *AargList, CSymbol *Askip, Zbool AsearchParent, Zbool Aconvert);
Zenum CSymbol__FgetReturnType(CSymbol *THIS);
Zbool CSymbol__FisPointerType(CSymbol *THIS);
Zbool CSymbol__FisMethodType(CSymbol *THIS);
CSymbol *CSymbol__X__Vunknown;
CSymbol *CSymbol__X__Vbool;
CSymbol *CSymbol__X__Vstatus;
CSymbol *CSymbol__X__Vint;
CSymbol *CSymbol__X__Vstring;
CSymbol *CSymbol__X__Vparent;
CSymbol *CSymbol__X__Varray;
CSymbol *CSymbol__X__Vnil;
CSymbol *CSymbol__X__Vproc_ref;
CSymbol *CSymbol__X__Vfunc_ref;
CSymbol *CSymbol__X__Ffind(CListHead *Alist, char *Aname);
CSymbol *CSymbol__X__Ffind__1(CListHead *Alist, char *Aname, Zenum Atype);
CSymbol *CSymbol__X__FfindMatchingFunctionInList(CListHead *AmemberList, char *Amethod, CListHead *AargList, CSymbol *Askip, Zbool  *RfoundSkip, Zbool Aconvert);
void Isymbol();
