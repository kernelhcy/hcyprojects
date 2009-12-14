#define INC_write_js_D 1
/*
 * DECLARE FUNCTIONS AND GLOBALS
 */
#ifndef INC_conversion_D
#include "../ZUDIR/conversion.d.h"
#endif
#ifndef INC_dictstuff_D
#include "../ZUDIR/dictstuff.d.h"
#endif
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
#ifndef INC_zimbufile_D
#include "../ZUDIR/zimbufile.d.h"
#endif
CWrite_JS *CWrite_JS__FNEW();
char *CWrite_JS__FgetLangName(CWrite_JS *THIS);
char *CWrite_JS__FthisName(CWrite_JS *THIS, Zbool AinsideNew);
CZimbuFile__X__CCodeSpecific *CWrite_JS__FgetCS(CWrite_JS *THIS, CZimbuFile *AzimbuFile);
void CWrite_JS__FmainHead(CWrite_JS *THIS, CSContext *Actx);
void CWrite_JS__FmainEnd(CWrite_JS *THIS, CNode *Anode, CSContext *Actx);
void CWrite_JS__FcopyC(CWrite_JS *THIS, CNode *Anode, CSContext *Actx);
void CWrite_JS__FcopyJS(CWrite_JS *THIS, CNode *Anode, CSContext *Actx);
void CWrite_JS__FwriteAlloc(CWrite_JS *THIS, char *AtypeName, CSContext *Actx);
void CWrite_JS__FwriteListAlloc(CWrite_JS *THIS, CSContext *Actx);
void CWrite_JS__FwriteNewThis(CWrite_JS *THIS, CSymbol *Asym, CSContext *Actx);
void CWrite_JS__FwriteNewReturn(CWrite_JS *THIS, COutput *Aout);
void CWrite_JS__FwriteSymName(CWrite_JS *THIS, CSymbol *Asym, CSContext *Actx);
CSymbol *CWrite_JS__Fid(CWrite_JS *THIS, CNode *Anode, CSContext *Actx, CSymbol *AdestSym);
CSymbol *CWrite_JS__FmethodReturnType(CWrite_JS *THIS, CNode *Anode, CSymbol *Asym, CSContext *Actx);
void CWrite_JS__FwriteMethodCall(CWrite_JS *THIS, CSymbol *Afunc, Zbool AmoreArgs, CSContext *Actx);
void CWrite_JS__FargWithType(CWrite_JS *THIS, Zbool Afirst, CSymbol *AtypeSym, CNode *AtypeNode, char *AargName, CSContext *Actx);
Zbool CWrite_JS__FdoWriteDecl(CWrite_JS *THIS);
CSymbol *CWrite_JS__Fsubscript(CWrite_JS *THIS, CNode *Anode, CSContext *Actx, CSymbol *AdestSym);
void CWrite_JS__FiobjectMember(CWrite_JS *THIS, CSymbol *AobjSym, CSymbol *AitfSym, CNode *Anode, CSContext *Actx, CSymbol *AdestSym);
void CWrite_JS__FnumberOp(CWrite_JS *THIS, CNode *Anode, CSContext *Actx);
void CWrite_JS__FconcatOp(CWrite_JS *THIS, CNode *Anode, CSContext *Actx);
Zenum CWrite_JS__FplusOp(CWrite_JS *THIS, CNode *Anode, CSContext *Actx, Zenum AdestType);
void CWrite_JS__FincrdecrOp(CWrite_JS *THIS, CNode *Anode, CSContext *Actx);
void CWrite_JS__FbooleanOp(CWrite_JS *THIS, CNode *Anode, CSContext *Actx);
void CWrite_JS__FcompareOp(CWrite_JS *THIS, CNode *Anode, CSContext *Actx);
void CWrite_JS__FandorOp(CWrite_JS *THIS, CNode *Anode, CSContext *Actx);
CSymbol *CWrite_JS__Fparens(CWrite_JS *THIS, CNode *Anode, CSContext *Actx, CSymbol *AdestSym);
CSymbol *CWrite_JS__FaltOp(CWrite_JS *THIS, CNode *Anode, CSContext *Actx, CSymbol *AdestSym);
CSymbol *CWrite_JS__FlistPart(CWrite_JS *THIS, CNode *Anode, CSContext *Actx, CSymbol *AdestSym);
void CWrite_JS__FdictPart(CWrite_JS *THIS, CNode *Anode, CSymbol *Aret, CSContext *Actx);
CSymbol *CWrite_JS__Fexpr(CWrite_JS *THIS, CNode *Anode, CSContext *Actx, CSymbol *AdestSym);
char *CWrite_JS__FwriteImport(CWrite_JS *THIS, CZimbuFile *Aimport, COutput__X__CGroup *AmyOuts, CScope *Ascope);
void CWrite_JS__FwriteIncludeImport(CWrite_JS *THIS, CZimbuFile *Aimport, COutput__X__CGroup *AmyOuts, CScope *Ascope);
Zbool CWrite_JS__FneedWrite(CWrite_JS *THIS, CZimbuFile *AzimbuFile);
void CWrite_JS__FwriteClassDef(CWrite_JS *THIS, char *Aname, COutput *AtypeOut);
void CWrite_JS__FwriteClassDecl(CWrite_JS *THIS, CSymbol *AclassSym, COutput__X__CGroup *Aouts, COutput *AstructOut);
void CWrite_JS__FdefaultInit(CWrite_JS *THIS, CSymbol *Asym, COutput *Aout);
void CWrite_JS__Fnil(CWrite_JS *THIS, CSContext *Actx);
void CWrite_JS__Fmember(CWrite_JS *THIS, char *Aname, COutput *Aout);
void CWrite_JS__Fvardecl(CWrite_JS *THIS, CSContext *Actx, COutput *Aout);
void CWrite_JS__Fvartype(CWrite_JS *THIS, CSymbol *Asym, CNode *Anode, CScope *Ascope, COutput *Aout);
void CWrite_JS__FforStart(CWrite_JS *THIS, COutput *Aout);
void CWrite_JS__FforLoop(CWrite_JS *THIS, CSymbol *AvarSym, COutput *Aout);
char *CWrite_JS__X__VnewThisName;
void CWrite_JS__X__FwriteDecl(CScope *AtopScope, FILE *Afd);
void CWrite_JS__X__FwriteBodies(CScope *AtopScope, FILE *Afd);
void Iwrite_js();
