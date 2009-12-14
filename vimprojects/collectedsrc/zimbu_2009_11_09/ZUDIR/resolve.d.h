#define INC_resolve_D 1
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
#ifndef INC_write_c_D
#include "../ZUDIR/write_c.d.h"
#endif
#ifndef INC_write_js_D
#include "../ZUDIR/write_js.d.h"
#endif
CResolve *CResolve__FNEW();
char *CResolve__FgetLangName(CResolve *THIS);
char *CResolve__FthisName(CResolve *THIS, Zbool AinsideNew);
CZimbuFile__X__CCodeSpecific *CResolve__FgetCS(CResolve *THIS, CZimbuFile *AzimbuFile);
void CResolve__FmainHead(CResolve *THIS, CSContext *Actx);
void CResolve__FmainEnd(CResolve *THIS, CNode *AmainNode, CSContext *Actx);
void CResolve__FcopyC(CResolve *THIS, CNode *Anode, CSContext *Actx);
void CResolve__FcopyJS(CResolve *THIS, CNode *Anode, CSContext *Actx);
void CResolve__FwriteAlloc(CResolve *THIS, char *AtypeName, CSContext *Actx);
void CResolve__FwriteListAlloc(CResolve *THIS, CSContext *Actx);
void CResolve__FwriteNewThis(CResolve *THIS, CSymbol *Asym, CSContext *Actx);
void CResolve__FwriteNewReturn(CResolve *THIS, COutput *Aout);
void CResolve__FwriteSymName(CResolve *THIS, CSymbol *Asym, CSContext *Actx);
CSymbol *CResolve__Fid(CResolve *THIS, CNode *Anode, CSContext *Actx, CSymbol *AdestSym);
CSymbol *CResolve__FmethodReturnType(CResolve *THIS, CNode *Anode, CSymbol *Asym, CSContext *Actx);
void CResolve__FwriteMethodCall(CResolve *THIS, CSymbol *Afunc, Zbool AmoreArgs, CSContext *Actx);
void CResolve__FargWithType(CResolve *THIS, Zbool Afirst, CSymbol *AtypeSym, CNode *AtypeNode, char *AargName, CSContext *Actx);
Zbool CResolve__FdoWriteDecl(CResolve *THIS);
CSymbol *CResolve__Fsubscript(CResolve *THIS, CNode *Anode, CSContext *Actx, CSymbol *AdestSym);
void CResolve__FiobjectMember(CResolve *THIS, CSymbol *AobjSym, CSymbol *AitfSym, CNode *Anode, CSContext *Actx, CSymbol *AdestSym);
void CResolve__FnumberOp(CResolve *THIS, CNode *Anode, CSContext *Actx);
void CResolve__FconcatOp(CResolve *THIS, CNode *Anode, CSContext *Actx);
Zenum CResolve__FplusOp(CResolve *THIS, CNode *Anode, CSContext *Actx, Zenum AdestType);
void CResolve__FincrdecrOp(CResolve *THIS, CNode *Anode, CSContext *Actx);
void CResolve__FbooleanOp(CResolve *THIS, CNode *Anode, CSContext *Actx);
void CResolve__FcompareOp(CResolve *THIS, CNode *Anode, CSContext *Actx);
void CResolve__FandorOp(CResolve *THIS, CNode *Anode, CSContext *Actx);
CSymbol *CResolve__Fparens(CResolve *THIS, CNode *Anode, CSContext *Actx, CSymbol *AdestSym);
CSymbol *CResolve__FaltOp(CResolve *THIS, CNode *Anode, CSContext *Actx, CSymbol *AdestSym);
CSymbol *CResolve__FlistPart(CResolve *THIS, CNode *Anode, CSContext *Actx, CSymbol *AdestSym);
void CResolve__FdictPart(CResolve *THIS, CNode *Anode, CSymbol *Aret, CSContext *Actx);
CSymbol *CResolve__Fexpr(CResolve *THIS, CNode *Anode, CSContext *Actx, CSymbol *AdestSym);
char *CResolve__FwriteImport(CResolve *THIS, CZimbuFile *Aimport, COutput__X__CGroup *Aouts, CScope *Ascope);
void CResolve__FwriteIncludeImport(CResolve *THIS, CZimbuFile *Aimport, COutput__X__CGroup *AmyOuts, CScope *Ascope);
Zbool CResolve__FneedWrite(CResolve *THIS, CZimbuFile *AzimbuFile);
void CResolve__FwriteClassDef(CResolve *THIS, char *Aname, COutput *AtypeOut);
void CResolve__FwriteClassDecl(CResolve *THIS, CSymbol *AclassSym, COutput__X__CGroup *Aouts, COutput *AstructOut);
void CResolve__FdefaultInit(CResolve *THIS, CSymbol *Asym, COutput *Aout);
void CResolve__Fnil(CResolve *THIS, CSContext *Actx);
void CResolve__Fmember(CResolve *THIS, char *Aname, COutput *Aout);
void CResolve__Fvardecl(CResolve *THIS, CSContext *Actx, COutput *Aout);
void CResolve__Fvartype(CResolve *THIS, CSymbol *Asym, CNode *Anode, CScope *Ascope, COutput *Aout);
void CResolve__FforStart(CResolve *THIS, COutput *Aout);
void CResolve__FforLoop(CResolve *THIS, CSymbol *AvarSym, COutput *Aout);
void  (*(CResolve_I__MforLoop__CSymbol__COutput_ptr[]))(void *, CSymbol *, COutput *) = {
  (void  (*)(void *, CSymbol *, COutput *))CResolve__FforLoop,
  (void  (*)(void *, CSymbol *, COutput *))CWrite_C__FforLoop,
  (void  (*)(void *, CSymbol *, COutput *))CWrite_JS__FforLoop,
};
void  (*(CResolve_I__MforStart__COutput_ptr[]))(void *, COutput *) = {
  (void  (*)(void *, COutput *))CResolve__FforStart,
  (void  (*)(void *, COutput *))CWrite_C__FforStart,
  (void  (*)(void *, COutput *))CWrite_JS__FforStart,
};
void  (*(CResolve_I__Mvartype__CSymbol__CNode__CScope__COutput_ptr[]))(void *, CSymbol *, CNode *, CScope *, COutput *) = {
  (void  (*)(void *, CSymbol *, CNode *, CScope *, COutput *))CResolve__Fvartype,
  (void  (*)(void *, CSymbol *, CNode *, CScope *, COutput *))CWrite_C__Fvartype,
  (void  (*)(void *, CSymbol *, CNode *, CScope *, COutput *))CWrite_JS__Fvartype,
};
void  (*(CResolve_I__Mvardecl__CSContext__COutput_ptr[]))(void *, CSContext *, COutput *) = {
  (void  (*)(void *, CSContext *, COutput *))CResolve__Fvardecl,
  (void  (*)(void *, CSContext *, COutput *))CWrite_C__Fvardecl,
  (void  (*)(void *, CSContext *, COutput *))CWrite_JS__Fvardecl,
};
void  (*(CResolve_I__Mmember__string__COutput_ptr[]))(void *, char *, COutput *) = {
  (void  (*)(void *, char *, COutput *))CResolve__Fmember,
  (void  (*)(void *, char *, COutput *))CWrite_C__Fmember,
  (void  (*)(void *, char *, COutput *))CWrite_JS__Fmember,
};
void  (*(CResolve_I__Mnil__CSContext_ptr[]))(void *, CSContext *) = {
  (void  (*)(void *, CSContext *))CResolve__Fnil,
  (void  (*)(void *, CSContext *))CWrite_C__Fnil,
  (void  (*)(void *, CSContext *))CWrite_JS__Fnil,
};
void  (*(CResolve_I__MdefaultInit__CSymbol__COutput_ptr[]))(void *, CSymbol *, COutput *) = {
  (void  (*)(void *, CSymbol *, COutput *))CResolve__FdefaultInit,
  (void  (*)(void *, CSymbol *, COutput *))CWrite_C__FdefaultInit,
  (void  (*)(void *, CSymbol *, COutput *))CWrite_JS__FdefaultInit,
};
void  (*(CResolve_I__MwriteClassDecl__CSymbol__COutput__X__CGroup__COutput_ptr[]))(void *, CSymbol *, COutput__X__CGroup *, COutput *) = {
  (void  (*)(void *, CSymbol *, COutput__X__CGroup *, COutput *))CResolve__FwriteClassDecl,
  (void  (*)(void *, CSymbol *, COutput__X__CGroup *, COutput *))CWrite_C__FwriteClassDecl,
  (void  (*)(void *, CSymbol *, COutput__X__CGroup *, COutput *))CWrite_JS__FwriteClassDecl,
};
void  (*(CResolve_I__MwriteClassDef__string__COutput_ptr[]))(void *, char *, COutput *) = {
  (void  (*)(void *, char *, COutput *))CResolve__FwriteClassDef,
  (void  (*)(void *, char *, COutput *))CWrite_C__FwriteClassDef,
  (void  (*)(void *, char *, COutput *))CWrite_JS__FwriteClassDef,
};
Zbool  (*(CResolve_I__MneedWrite__CZimbuFile_ptr[]))(void *, CZimbuFile *) = {
  (Zbool  (*)(void *, CZimbuFile *))CResolve__FneedWrite,
  (Zbool  (*)(void *, CZimbuFile *))CWrite_C__FneedWrite,
  (Zbool  (*)(void *, CZimbuFile *))CWrite_JS__FneedWrite,
};
void  (*(CResolve_I__MwriteIncludeImport__CZimbuFile__COutput__X__CGroup__CScope_ptr[]))(void *, CZimbuFile *, COutput__X__CGroup *, CScope *) = {
  (void  (*)(void *, CZimbuFile *, COutput__X__CGroup *, CScope *))CResolve__FwriteIncludeImport,
  (void  (*)(void *, CZimbuFile *, COutput__X__CGroup *, CScope *))CWrite_C__FwriteIncludeImport,
  (void  (*)(void *, CZimbuFile *, COutput__X__CGroup *, CScope *))CWrite_JS__FwriteIncludeImport,
};
char * (*(CResolve_I__MwriteImport__CZimbuFile__COutput__X__CGroup__CScope_ptr[]))(void *, CZimbuFile *, COutput__X__CGroup *, CScope *) = {
  (char * (*)(void *, CZimbuFile *, COutput__X__CGroup *, CScope *))CResolve__FwriteImport,
  (char * (*)(void *, CZimbuFile *, COutput__X__CGroup *, CScope *))CWrite_C__FwriteImport,
  (char * (*)(void *, CZimbuFile *, COutput__X__CGroup *, CScope *))CWrite_JS__FwriteImport,
};
CSymbol * (*(CResolve_I__Mexpr__CNode__CSContext__CSymbol_ptr[]))(void *, CNode *, CSContext *, CSymbol *) = {
  (CSymbol * (*)(void *, CNode *, CSContext *, CSymbol *))CResolve__Fexpr,
  (CSymbol * (*)(void *, CNode *, CSContext *, CSymbol *))CWrite_C__Fexpr,
  (CSymbol * (*)(void *, CNode *, CSContext *, CSymbol *))CWrite_JS__Fexpr,
};
void  (*(CResolve_I__MdictPart__CNode__CSymbol__CSContext_ptr[]))(void *, CNode *, CSymbol *, CSContext *) = {
  (void  (*)(void *, CNode *, CSymbol *, CSContext *))CResolve__FdictPart,
  (void  (*)(void *, CNode *, CSymbol *, CSContext *))CWrite_C__FdictPart,
  (void  (*)(void *, CNode *, CSymbol *, CSContext *))CWrite_JS__FdictPart,
};
CSymbol * (*(CResolve_I__MlistPart__CNode__CSContext__CSymbol_ptr[]))(void *, CNode *, CSContext *, CSymbol *) = {
  (CSymbol * (*)(void *, CNode *, CSContext *, CSymbol *))CResolve__FlistPart,
  (CSymbol * (*)(void *, CNode *, CSContext *, CSymbol *))CWrite_C__FlistPart,
  (CSymbol * (*)(void *, CNode *, CSContext *, CSymbol *))CWrite_JS__FlistPart,
};
CSymbol * (*(CResolve_I__MaltOp__CNode__CSContext__CSymbol_ptr[]))(void *, CNode *, CSContext *, CSymbol *) = {
  (CSymbol * (*)(void *, CNode *, CSContext *, CSymbol *))CResolve__FaltOp,
  (CSymbol * (*)(void *, CNode *, CSContext *, CSymbol *))CWrite_C__FaltOp,
  (CSymbol * (*)(void *, CNode *, CSContext *, CSymbol *))CWrite_JS__FaltOp,
};
CSymbol * (*(CResolve_I__Mparens__CNode__CSContext__CSymbol_ptr[]))(void *, CNode *, CSContext *, CSymbol *) = {
  (CSymbol * (*)(void *, CNode *, CSContext *, CSymbol *))CResolve__Fparens,
  (CSymbol * (*)(void *, CNode *, CSContext *, CSymbol *))CWrite_C__Fparens,
  (CSymbol * (*)(void *, CNode *, CSContext *, CSymbol *))CWrite_JS__Fparens,
};
void  (*(CResolve_I__MandorOp__CNode__CSContext_ptr[]))(void *, CNode *, CSContext *) = {
  (void  (*)(void *, CNode *, CSContext *))CResolve__FandorOp,
  (void  (*)(void *, CNode *, CSContext *))CWrite_C__FandorOp,
  (void  (*)(void *, CNode *, CSContext *))CWrite_JS__FandorOp,
};
void  (*(CResolve_I__McompareOp__CNode__CSContext_ptr[]))(void *, CNode *, CSContext *) = {
  (void  (*)(void *, CNode *, CSContext *))CResolve__FcompareOp,
  (void  (*)(void *, CNode *, CSContext *))CWrite_C__FcompareOp,
  (void  (*)(void *, CNode *, CSContext *))CWrite_JS__FcompareOp,
};
void  (*(CResolve_I__MbooleanOp__CNode__CSContext_ptr[]))(void *, CNode *, CSContext *) = {
  (void  (*)(void *, CNode *, CSContext *))CResolve__FbooleanOp,
  (void  (*)(void *, CNode *, CSContext *))CWrite_C__FbooleanOp,
  (void  (*)(void *, CNode *, CSContext *))CWrite_JS__FbooleanOp,
};
void  (*(CResolve_I__MincrdecrOp__CNode__CSContext_ptr[]))(void *, CNode *, CSContext *) = {
  (void  (*)(void *, CNode *, CSContext *))CResolve__FincrdecrOp,
  (void  (*)(void *, CNode *, CSContext *))CWrite_C__FincrdecrOp,
  (void  (*)(void *, CNode *, CSContext *))CWrite_JS__FincrdecrOp,
};
Zenum  (*(CResolve_I__MplusOp__CNode__CSContext__CNode__X__EType_ptr[]))(void *, CNode *, CSContext *, Zenum ) = {
  (Zenum  (*)(void *, CNode *, CSContext *, Zenum ))CResolve__FplusOp,
  (Zenum  (*)(void *, CNode *, CSContext *, Zenum ))CWrite_C__FplusOp,
  (Zenum  (*)(void *, CNode *, CSContext *, Zenum ))CWrite_JS__FplusOp,
};
void  (*(CResolve_I__MconcatOp__CNode__CSContext_ptr[]))(void *, CNode *, CSContext *) = {
  (void  (*)(void *, CNode *, CSContext *))CResolve__FconcatOp,
  (void  (*)(void *, CNode *, CSContext *))CWrite_C__FconcatOp,
  (void  (*)(void *, CNode *, CSContext *))CWrite_JS__FconcatOp,
};
void  (*(CResolve_I__MnumberOp__CNode__CSContext_ptr[]))(void *, CNode *, CSContext *) = {
  (void  (*)(void *, CNode *, CSContext *))CResolve__FnumberOp,
  (void  (*)(void *, CNode *, CSContext *))CWrite_C__FnumberOp,
  (void  (*)(void *, CNode *, CSContext *))CWrite_JS__FnumberOp,
};
void  (*(CResolve_I__MiobjectMember__CSymbol__CSymbol__CNode__CSContext__CSymbol_ptr[]))(void *, CSymbol *, CSymbol *, CNode *, CSContext *, CSymbol *) = {
  (void  (*)(void *, CSymbol *, CSymbol *, CNode *, CSContext *, CSymbol *))CResolve__FiobjectMember,
  (void  (*)(void *, CSymbol *, CSymbol *, CNode *, CSContext *, CSymbol *))CWrite_C__FiobjectMember,
  (void  (*)(void *, CSymbol *, CSymbol *, CNode *, CSContext *, CSymbol *))CWrite_JS__FiobjectMember,
};
CSymbol * (*(CResolve_I__Msubscript__CNode__CSContext__CSymbol_ptr[]))(void *, CNode *, CSContext *, CSymbol *) = {
  (CSymbol * (*)(void *, CNode *, CSContext *, CSymbol *))CResolve__Fsubscript,
  (CSymbol * (*)(void *, CNode *, CSContext *, CSymbol *))CWrite_C__Fsubscript,
  (CSymbol * (*)(void *, CNode *, CSContext *, CSymbol *))CWrite_JS__Fsubscript,
};
Zbool  (*(CResolve_I__MdoWriteDecl_ptr[]))(void *) = {
  (Zbool  (*)(void *))CResolve__FdoWriteDecl,
  (Zbool  (*)(void *))CWrite_C__FdoWriteDecl,
  (Zbool  (*)(void *))CWrite_JS__FdoWriteDecl,
};
void  (*(CResolve_I__MargWithType__bool__CSymbol__CNode__string__CSContext_ptr[]))(void *, Zbool , CSymbol *, CNode *, char *, CSContext *) = {
  (void  (*)(void *, Zbool , CSymbol *, CNode *, char *, CSContext *))CResolve__FargWithType,
  (void  (*)(void *, Zbool , CSymbol *, CNode *, char *, CSContext *))CWrite_C__FargWithType,
  (void  (*)(void *, Zbool , CSymbol *, CNode *, char *, CSContext *))CWrite_JS__FargWithType,
};
void  (*(CResolve_I__MwriteMethodCall__CSymbol__bool__CSContext_ptr[]))(void *, CSymbol *, Zbool , CSContext *) = {
  (void  (*)(void *, CSymbol *, Zbool , CSContext *))CResolve__FwriteMethodCall,
  (void  (*)(void *, CSymbol *, Zbool , CSContext *))CWrite_C__FwriteMethodCall,
  (void  (*)(void *, CSymbol *, Zbool , CSContext *))CWrite_JS__FwriteMethodCall,
};
CSymbol * (*(CResolve_I__MmethodReturnType__CNode__CSymbol__CSContext_ptr[]))(void *, CNode *, CSymbol *, CSContext *) = {
  (CSymbol * (*)(void *, CNode *, CSymbol *, CSContext *))CResolve__FmethodReturnType,
  (CSymbol * (*)(void *, CNode *, CSymbol *, CSContext *))CWrite_C__FmethodReturnType,
  (CSymbol * (*)(void *, CNode *, CSymbol *, CSContext *))CWrite_JS__FmethodReturnType,
};
CSymbol * (*(CResolve_I__Mid__CNode__CSContext__CSymbol_ptr[]))(void *, CNode *, CSContext *, CSymbol *) = {
  (CSymbol * (*)(void *, CNode *, CSContext *, CSymbol *))CResolve__Fid,
  (CSymbol * (*)(void *, CNode *, CSContext *, CSymbol *))CWrite_C__Fid,
  (CSymbol * (*)(void *, CNode *, CSContext *, CSymbol *))CWrite_JS__Fid,
};
void  (*(CResolve_I__MwriteSymName__CSymbol__CSContext_ptr[]))(void *, CSymbol *, CSContext *) = {
  (void  (*)(void *, CSymbol *, CSContext *))CResolve__FwriteSymName,
  (void  (*)(void *, CSymbol *, CSContext *))CWrite_C__FwriteSymName,
  (void  (*)(void *, CSymbol *, CSContext *))CWrite_JS__FwriteSymName,
};
void  (*(CResolve_I__MwriteNewReturn__COutput_ptr[]))(void *, COutput *) = {
  (void  (*)(void *, COutput *))CResolve__FwriteNewReturn,
  (void  (*)(void *, COutput *))CWrite_C__FwriteNewReturn,
  (void  (*)(void *, COutput *))CWrite_JS__FwriteNewReturn,
};
void  (*(CResolve_I__MwriteNewThis__CSymbol__CSContext_ptr[]))(void *, CSymbol *, CSContext *) = {
  (void  (*)(void *, CSymbol *, CSContext *))CResolve__FwriteNewThis,
  (void  (*)(void *, CSymbol *, CSContext *))CWrite_C__FwriteNewThis,
  (void  (*)(void *, CSymbol *, CSContext *))CWrite_JS__FwriteNewThis,
};
void  (*(CResolve_I__MwriteListAlloc__CSContext_ptr[]))(void *, CSContext *) = {
  (void  (*)(void *, CSContext *))CResolve__FwriteListAlloc,
  (void  (*)(void *, CSContext *))CWrite_C__FwriteListAlloc,
  (void  (*)(void *, CSContext *))CWrite_JS__FwriteListAlloc,
};
void  (*(CResolve_I__MwriteAlloc__string__CSContext_ptr[]))(void *, char *, CSContext *) = {
  (void  (*)(void *, char *, CSContext *))CResolve__FwriteAlloc,
  (void  (*)(void *, char *, CSContext *))CWrite_C__FwriteAlloc,
  (void  (*)(void *, char *, CSContext *))CWrite_JS__FwriteAlloc,
};
void  (*(CResolve_I__McopyJS__CNode__CSContext_ptr[]))(void *, CNode *, CSContext *) = {
  (void  (*)(void *, CNode *, CSContext *))CResolve__FcopyJS,
  (void  (*)(void *, CNode *, CSContext *))CWrite_C__FcopyJS,
  (void  (*)(void *, CNode *, CSContext *))CWrite_JS__FcopyJS,
};
void  (*(CResolve_I__McopyC__CNode__CSContext_ptr[]))(void *, CNode *, CSContext *) = {
  (void  (*)(void *, CNode *, CSContext *))CResolve__FcopyC,
  (void  (*)(void *, CNode *, CSContext *))CWrite_C__FcopyC,
  (void  (*)(void *, CNode *, CSContext *))CWrite_JS__FcopyC,
};
void  (*(CResolve_I__MmainEnd__CNode__CSContext_ptr[]))(void *, CNode *, CSContext *) = {
  (void  (*)(void *, CNode *, CSContext *))CResolve__FmainEnd,
  (void  (*)(void *, CNode *, CSContext *))CWrite_C__FmainEnd,
  (void  (*)(void *, CNode *, CSContext *))CWrite_JS__FmainEnd,
};
void  (*(CResolve_I__MmainHead__CSContext_ptr[]))(void *, CSContext *) = {
  (void  (*)(void *, CSContext *))CResolve__FmainHead,
  (void  (*)(void *, CSContext *))CWrite_C__FmainHead,
  (void  (*)(void *, CSContext *))CWrite_JS__FmainHead,
};
CZimbuFile__X__CCodeSpecific * (*(CResolve_I__MgetCS__CZimbuFile_ptr[]))(void *, CZimbuFile *) = {
  (CZimbuFile__X__CCodeSpecific * (*)(void *, CZimbuFile *))CResolve__FgetCS,
  (CZimbuFile__X__CCodeSpecific * (*)(void *, CZimbuFile *))CWrite_C__FgetCS,
  (CZimbuFile__X__CCodeSpecific * (*)(void *, CZimbuFile *))CWrite_JS__FgetCS,
};
char * (*(CResolve_I__MthisName__bool_ptr[]))(void *, Zbool ) = {
  (char * (*)(void *, Zbool ))CResolve__FthisName,
  (char * (*)(void *, Zbool ))CWrite_C__FthisName,
  (char * (*)(void *, Zbool ))CWrite_JS__FthisName,
};
char * (*(CResolve_I__MgetLangName_ptr[]))(void *) = {
  (char * (*)(void *))CResolve__FgetLangName,
  (char * (*)(void *))CWrite_C__FgetLangName,
  (char * (*)(void *))CWrite_JS__FgetLangName,
};
CResolve * (*(CResolve_I__MNEW_ptr[]))() = {
  (CResolve * (*)())CResolve__FNEW,
  (CResolve * (*)())CWrite_C__FNEW,
  (CResolve * (*)())CWrite_JS__FNEW,
};
int CResolve_I__VpermuName_off[] = {
  (char *)&((CResolve *)&dummy)->VpermuName - (char *)(CResolve *)&dummy,
  (char *)&((CWrite_C *)&dummy)->VpermuName - (char *)(CWrite_C *)&dummy,
  (char *)&((CWrite_JS *)&dummy)->VpermuName - (char *)(CWrite_JS *)&dummy,
};
int CResolve_I__VtargetLang_off[] = {
  (char *)&((CResolve *)&dummy)->VtargetLang - (char *)(CResolve *)&dummy,
  (char *)&((CWrite_C *)&dummy)->VtargetLang - (char *)(CWrite_C *)&dummy,
  (char *)&((CWrite_JS *)&dummy)->VtargetLang - (char *)(CWrite_JS *)&dummy,
};
void Iresolve();
