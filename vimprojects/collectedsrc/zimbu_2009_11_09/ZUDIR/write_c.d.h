#define INC_write_c_D 1
/*
 * DECLARE FUNCTIONS AND GLOBALS
 */
#ifndef INC_config_D
#include "../ZUDIR/config.d.h"
#endif
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
#ifndef INC_libarg_D
#include "../lib/ZUDIR/libarg.d.h"
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
CWrite_C *CWrite_C__FNEW();
char *CWrite_C__FthisName(CWrite_C *THIS, Zbool AinsideNew);
char *CWrite_C__FgetLangName(CWrite_C *THIS);
CZimbuFile__X__CCodeSpecific *CWrite_C__FgetCS(CWrite_C *THIS, CZimbuFile *AzimbuFile);
Zbool CWrite_C__FtoplevelLines(CWrite_C *THIS, CZimbuFile *AzimbuFile);
void CWrite_C__FmainHead(CWrite_C *THIS, CSContext *Actx);
void CWrite_C__FmainEnd(CWrite_C *THIS, CNode *AmainNode, CSContext *Actx);
void CWrite_C__FcopyC(CWrite_C *THIS, CNode *Anode, CSContext *Actx);
void CWrite_C__FcopyJS(CWrite_C *THIS, CNode *Anode, CSContext *Actx);
void CWrite_C__FwriteAlloc(CWrite_C *THIS, char *AtypeName, CSContext *Actx);
void CWrite_C__FwriteListAlloc(CWrite_C *THIS, CSContext *Actx);
void CWrite_C__FwriteNewThis(CWrite_C *THIS, CSymbol *Asym, CSContext *Actx);
void CWrite_C__FwriteNewReturn(CWrite_C *THIS, COutput *Aout);
void CWrite_C__FwriteSymName(CWrite_C *THIS, CSymbol *Asym, CSContext *Actx);
CSymbol *CWrite_C__Fid(CWrite_C *THIS, CNode *Anode, CSContext *Actx, CSymbol *AdestSym);
CSymbol *CWrite_C__FmethodReturnType(CWrite_C *THIS, CNode *Anode, CSymbol *Asym, CSContext *Actx);
void CWrite_C__FwriteMethodCall(CWrite_C *THIS, CSymbol *Afunc, Zbool AmoreArgs, CSContext *Actx);
void CWrite_C__FargWithType(CWrite_C *THIS, Zbool Afirst, CSymbol *AtypeSym, CNode *AtypeNode, char *AargName, CSContext *Actx);
Zbool CWrite_C__FdoWriteDecl(CWrite_C *THIS);
CSymbol *CWrite_C__Fsubscript(CWrite_C *THIS, CNode *Anode, CSContext *Actx, CSymbol *AdestSym);
void CWrite_C__FiobjectMember(CWrite_C *THIS, CSymbol *AobjSym, CSymbol *AitfSym, CNode *Anode, CSContext *Actx, CSymbol *AdestSym);
void CWrite_C__FnumberOp(CWrite_C *THIS, CNode *Anode, CSContext *Actx);
void CWrite_C__FconcatOp(CWrite_C *THIS, CNode *Anode, CSContext *Actx);
Zenum CWrite_C__FplusOp(CWrite_C *THIS, CNode *Anode, CSContext *Actx, Zenum AdestType);
void CWrite_C__FincrdecrOp(CWrite_C *THIS, CNode *Anode, CSContext *Actx);
void CWrite_C__FbooleanOp(CWrite_C *THIS, CNode *Anode, CSContext *Actx);
void CWrite_C__FcompareOp(CWrite_C *THIS, CNode *Anode, CSContext *Actx);
void CWrite_C__FandorOp(CWrite_C *THIS, CNode *Anode, CSContext *Actx);
CSymbol *CWrite_C__Fparens(CWrite_C *THIS, CNode *Anode, CSContext *Actx, CSymbol *AdestSym);
CSymbol *CWrite_C__FaltOp(CWrite_C *THIS, CNode *Anode, CSContext *Actx, CSymbol *AdestSym);
CSymbol *CWrite_C__FlistPart(CWrite_C *THIS, CNode *Anode, CSContext *Actx, CSymbol *AdestSym);
void CWrite_C__FdictPart(CWrite_C *THIS, CNode *Anode, CSymbol *Aret, CSContext *Actx);
CSymbol *CWrite_C__Fexpr(CWrite_C *THIS, CNode *Anode, CSContext *Actx, CSymbol *AdestSym);
char *CWrite_C__FwriteImport(CWrite_C *THIS, CZimbuFile *Aimport, COutput__X__CGroup *AdummyOuts, CScope *Ascope);
void CWrite_C__FwriteImportFile(CWrite_C *THIS, char *Afname, COutput__X__CFragmentHead *Ahead, char *Aheader, char *Adef);
void CWrite_C__FwriteIncludeImport(CWrite_C *THIS, CZimbuFile *Aimport, COutput__X__CGroup *AmyOuts, CScope *Ascope);
Zbool CWrite_C__FneedWrite(CWrite_C *THIS, CZimbuFile *AzimbuFile);
void CWrite_C__FwriteClassDef(CWrite_C *THIS, char *Aname, COutput *AtypeOut);
void CWrite_C__FwriteClassDecl(CWrite_C *THIS, CSymbol *AclassSym, COutput__X__CGroup *Aouts, COutput *AstructOut);
void CWrite_C__FdefaultInit(CWrite_C *THIS, CSymbol *Asym, COutput *Aout);
void CWrite_C__Fnil(CWrite_C *THIS, CSContext *Actx);
void CWrite_C__Fmember(CWrite_C *THIS, char *Aname, COutput *Aout);
void CWrite_C__Fvardecl(CWrite_C *THIS, CSContext *Actx, COutput *Aout);
void CWrite_C__Fvartype(CWrite_C *THIS, CSymbol *Asym, CNode *Anode, CScope *Ascope, COutput *Aout);
void CWrite_C__FforStart(CWrite_C *THIS, COutput *Aout);
void CWrite_C__FforLoop(CWrite_C *THIS, CSymbol *AvarSym, COutput *Aout);
char *CWrite_C__X__VnewThisName;
CListHead *CWrite_C__X__VincludeWriters;
CListHead *CWrite_C__X__VtypedefWriters;
CListHead *CWrite_C__X__VdeclWriters;
CListHead *CWrite_C__X__VbodyWriters;
void CWrite_C__X__FwriteFile(COutput__X__CHeads *Aheads, CScope *AtopScope, FILE *AoutFile);
void CWrite_C__X__FwriteIncludes(CScope *AtopScope, FILE *Afd);
void CWrite_C__X__FwriteTypedefs(CScope *AtopScope, FILE *Afd);
void CWrite_C__X__FwriteDecl(CScope *AtopScope, FILE *Afd);
void CWrite_C__X__FwriteBodies(CScope *AtopScope, FILE *Afd);
void Iwrite_c();
