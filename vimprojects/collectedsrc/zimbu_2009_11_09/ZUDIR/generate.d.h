#define INC_generate_D 1
/*
 * DECLARE FUNCTIONS AND GLOBALS
 */
#ifndef INC_builtin_D
#include "../ZUDIR/builtin.d.h"
#endif
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
#ifndef INC_expreval_D
#include "../ZUDIR/expreval.d.h"
#endif
#ifndef INC_input_D
#include "../ZUDIR/input.d.h"
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
#ifndef INC_parse_D
#include "../ZUDIR/parse.d.h"
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
#ifndef INC_usedfile_D
#include "../ZUDIR/usedfile.d.h"
#endif
#ifndef INC_write_c_D
#include "../ZUDIR/write_c.d.h"
#endif
#ifndef INC_write_js_D
#include "../ZUDIR/write_js.d.h"
#endif
#ifndef INC_zimbufile_D
#include "../ZUDIR/zimbufile.d.h"
#endif
#ifndef INC_zwtvalues_D
#include "../lib/ZUDIR/zwtvalues.d.h"
#endif
Zint MGenerate__VscopeNumber;
Zbool MGenerate__VshowErrors;
COutput *MGenerate__VnoOut;
Zbool MGenerate__Vskip_zero_undefined;
Zbool MGenerate__FdoError(COutput *Aout);
Zbool MGenerate__Fresolve(CUsedFile *AusedFile);
Zbool MGenerate__Fresolve__1(CUsedFile *AusedFile, char *Aindent);
void MGenerate__Fwrite(CUsedFile *AusedFile, COutput__X__CGroup *Aoutputs);
void MGenerate__Fwrite__1(CUsedFile *AusedFile, Zoref *Agen, COutput__X__CGroup *Aoutputs);
Zint MGenerate__FgenerateImports(CUsedFile *AusedFile, Zoref *Agen, COutput__X__CGroup *Aouts);
Zint MGenerate__Fgenerate(CNode *AstartNode, CScope *Ascope, Zoref *Agen, COutput__X__CGroup *Aouts);
Zint MGenerate__FgenerateNode(CNode *AstartNode, CScope *Ascope, Zoref *Agen, COutput__X__CGroup *Aouts);
Zint MGenerate__FgenerateOneNode(CNode * *Rref, CSContext *Actx, COutput__X__CGroup *Aouts);
Zint MGenerate__FgenerateMain(CNode *Anode, CSContext *Actx, COutput__X__CGroup *Aouts);
Zint MGenerate__FgenerateModule(CNode *Anode, CSContext *Actx, COutput__X__CGroup *Aouts);
Zint MGenerate__FgenerateClass(CNode *Anode, CSContext *Actx, COutput__X__CGroup *Aouts);
Zint MGenerate__FgenerateShared(CNode *Anode, CSContext *Actx, COutput__X__CGroup *Aouts);
Zint MGenerate__FgenerateBits(CNode *Anode, CSContext *Actx, COutput__X__CGroup *Aouts);
Zint MGenerate__FgenerateEnum(CNode *Anode, CSContext *Actx, COutput__X__CGroup *Aouts);
Zint MGenerate__FgenerateFor(CNode *Anode, CSContext *Actx, COutput__X__CGroup *Aouts);
Zint MGenerate__FgenerateReturnExit(CNode *Anode, CSContext *Actx, COutput__X__CGroup *Aouts);
Zint MGenerate__FgenerateSwitch(CNode *Anode, CSContext *Actx, COutput__X__CGroup *Aouts);
Zint MGenerate__FgenerateCase(CNode * *Rref, CSContext *Actx, COutput__X__CGroup *Aouts);
CSymbol *MGenerate__FgenerateCall(CNode *Am_node, CSContext *Actx, CSymbol *AdestSym, Zbool AstrictType);
Zint MGenerate__FgenerateIncDec(CNode *Anode, CSContext *Actx, COutput__X__CGroup *Aouts);
Zint MGenerate__FgenerateAssign(CNode *Anode, CSContext *Actx, COutput__X__CGroup *Aouts);
Zint MGenerate__FgenerateDictAssign(CNode *Anode, CSymbol *Asp, CSContext *Actx, COutput__X__CGroup *Aouts);
Zint MGenerate__FgenerateMethod(CNode *Anode, CSContext *Actx, COutput__X__CGroup *Aouts);
Zint MGenerate__FgenerateDeclare(CNode *Anode, CSContext *Actx, COutput__X__CGroup *Aouts);
Zint MGenerate__FgenerateGenerateIf(CNode *Anode, CSContext *Actx, COutput__X__CGroup *Aouts);
Zint MGenerate__FhandleImplements(CNode *Anode, CSymbol *Aclass, CScope *Ascope, Zbool AgiveError);
Zint MGenerate__FgenerateScope(CNode *Anode, CSContext *Actx, Zenum AnodeType, CSymbol *AswitchSymbol, COutput__X__CGroup *Aouts);
void MGenerate__FgenerateClassOffTable(CSymbol *AclassSym, CSContext *Actx);
void MGenerate__FgenClassOffTableList(CSymbol *AclassSym, CSymbol *AloopSym, COutput *Aout, Zbool AdoMethods);
void MGenerate__FgenMethodChildrenList(CSymbol *Aclass, CSymbol *Amember, COutput *Aout);
void MGenerate__FgenMemberChildrenList(CSymbol *Aclass, CSymbol *Amember, COutput *Aout);
void MGenerate__FmethodTypeCast(CSymbol *Am, COutput *Aout);
char *MGenerate__FclassFuncTableName(CSymbol *AclassSym, CSymbol *Amember);
char *MGenerate__FclassOffTableName(CSymbol *AclassSym, CSymbol *Amember);
void MGenerate__FgenerateMethodReturnType(CSymbol *Asym, COutput *Aout);
void MGenerate__FgenerateMethodArgTypes(CSymbol *Asym, COutput *Aout);
CListHead *MGenerate__VimportedFiles;
Zint MGenerate__FgenerateImport(CUsedFile *AusedFile, CNode *Anode, Zoref *Agen, COutput__X__CGroup *Aouts);
void MGenerate__FhandleImport(CUsedFile *AusedFile, CNode *Anode);
Zint MGenerate__FprocessImport(CUsedFile *AusedFile, Zoref *Agen, CScope *Ascope, COutput__X__CGroup *Aouts);
void MGenerate__FgenerateJSFile(CUsedFile *AusedFile, CScope *Ascope);
void MGenerate__FcheckScope(CNode *Anode, CScope *Ascope, Zbool AisBreak);
CSymbol *MGenerate__FgenerateObjDeclType(CNode *Anode, CSContext *Actx);
CSymbol *MGenerate__FgenerateDeclType(CNode *Anode, CSContext *Actx);
CSymbol *MGenerate__FgenerateContainerType(CNode *Anode, CSContext *Actx);
CSymbol *MGenerate__FgenerateMethodCall(CNode *Am_node, CSContext *Actx, CSymbol *AdestSym);
void MGenerate__FgenEnumNameCall(CSymbol *AenumSym, CNode *Anode, CSContext *Actx);
CSymbol *MGenerate__FgenerateObjectCall(CSymbol *Asp, CNode *Am_node, CSContext *Actx, CSymbol *AdestSym);
CDictHead *MGenerate__VvirtualFuncMap;
COutput *MGenerate__VvirtualOut;
CSymbol *MGenerate__FgenerateVirtualFunc(char *Aname, CSymbol *AvarSym, CNode *Anode, CListHead *Aarglist, CSContext *Actx);
CListHead *MGenerate__FinterfaceClassList(CSymbol *Asym);
Zbool MGenerate__FclassOfInterface(CSymbol *Aitf, CSymbol *Achild);
CSymbol *MGenerate__FfindMethod(CSymbol *Aparent, char *Aname, CNode *Aargs, CSContext *Actx, Zbool AnoneOK, CNode *AmsgNode, char *Amsg);
CSymbol *MGenerate__FfindMethodArglist(CSymbol *Aparent, char *Aname, CListHead *Aarglist, CScope *Ascope, Zbool AnoneOK, CNode *AmsgNode, char *Amsg);
void MGenerate__FlistMatchingMethods(char *Aname, CSymbol *Aparent, CListHead *AargTypeList);
void MGenerate__FlistArgTypes(CListHead *AargList);
char *MGenerate__FargTypesAsString(CListHead *AargList);
void MGenerate__FgenerateRefCast(CSymbol *Asym, CNode *Anode, COutput *Aout);
CListHead *MGenerate__FgetSymbolListFromArgNode(CNode *Anode, CSContext *Actx, Zbits Agsarg);
CSymbol *MGenerate__FgenerateNewCall(CNode *Anode, CSContext *Actx, CSymbol *AdestSym);
void MGenerate__FlistMatchingFunc(CScope *Ascope, char *AfuncName, CListHead *AargTypeList, Zbool Aall);
CSymbol *MGenerate__FgenerateFunctionCall(CSymbol *Afunc, CNode *Anode, CSContext *Actx, Zenum Adest_type);
void MGenerate__FgenerateArgumentsCheck(CNode *Anode, char *AfuncName, CSContext *Actx, CListHead *Aargs);
Zint MGenerate__FgenerateArguments(CNode *Anode, CSContext *Actx, CListHead *Aargs, Zint Aindex);
CSymbol *MGenerate__FgenerateModuleCall(CSymbol *Asym, CNode *Aarg_node, CSContext *Actx);
CSymbol *MGenerate__FgenerateModuleInfo(CSymbol *Asym, CNode *Anode, CSContext *Actx);
CSymbol *MGenerate__FgenerateVarnameParent(CNode *Anode, CSContext *Actx, CSymbol *AdestSym);
CSymbol *MGenerate__FgenerateVarname(CNode *Anode, CSContext *Actx, CSymbol *AdestSym);
CSymbol *MGenerate__FgenerateLVarname(CNode *Anode, Zbool Alvalue, CSContext *Actx, CSymbol *AdestSym);
CSymbol *MGenerate__FgenerateVarnamePart(CNode *Anode, Zbool Alvalue, CSContext *Actx, CSymbol *AdestSym);
Zbool MGenerate__FcompatibleTypes(Zenum Asrc_type, Zenum Adest_type);
Zbool MGenerate__FcompatibleSymbols(CSymbol *Asrc, CSymbol *Adest);
CSymbol *MGenerate__FgenExpr(CNode *Anode, CSContext *Actx, CSymbol *AdestSym);
CSymbol *MGenerate__FgenExprChecked(CNode *Anode, CSContext *Actx, CSymbol *AdestSym);
void MGenerate__FgenerateString(CNode *Anode, CSContext *Actx);
void MGenerate__FgenEnumMember(CNode *Anode, CSymbol *AnodeSym, CSymbol *AdestSym, COutput *Aout, CSymbol *Aret);
void MGenerate__FgenBitsMember(CNode *Anode, CSymbol *AnodeSym, Zenum Adest_type, CSContext *Actx, CSymbol *Aret);
CSymbol *MGenerate__FgenModuleMember(CNode *Anode, Zenum Adest_type, CSContext *Actx, CSymbol *Aret_in);
CSymbol *MGenerate__FgenClassMember(CNode *Anode, Zenum Adest_type, CSContext *Actx, CSymbol *Aret_in);
void MGenerate__Ferror(char *Amsg, CNode *Anode);
void MGenerate__FtypeError(Zenum Aexpected, Zenum Aactual, CNode *Anode);
void MGenerate__FcheckExprType(Zenum Aexpected, Zenum Aactual, CNode *Anode);
void MGenerate__FgenType(CSymbol *Asym, CNode *Anode, COutput *Aout);
void Igenerate();
