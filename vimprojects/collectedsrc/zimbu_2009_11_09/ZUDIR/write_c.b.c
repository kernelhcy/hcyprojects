#define INC_write_c_B 1
/*
 * FUNCTION BODIES
 */
#ifndef INC_config_B
#include "../ZUDIR/config.b.c"
#endif
#ifndef INC_conversion_B
#include "../ZUDIR/conversion.b.c"
#endif
#ifndef INC_dictstuff_B
#include "../ZUDIR/dictstuff.b.c"
#endif
#ifndef INC_error_B
#include "../ZUDIR/error.b.c"
#endif
#ifndef INC_generate_B
#include "../ZUDIR/generate.b.c"
#endif
#ifndef INC_libarg_B
#include "../lib/ZUDIR/libarg.b.c"
#endif
#ifndef INC_liststuff_B
#include "../ZUDIR/liststuff.b.c"
#endif
#ifndef INC_node_B
#include "../ZUDIR/node.b.c"
#endif
#ifndef INC_output_B
#include "../ZUDIR/output.b.c"
#endif
#ifndef INC_resolve_B
#include "../ZUDIR/resolve.b.c"
#endif
#ifndef INC_scontext_B
#include "../ZUDIR/scontext.b.c"
#endif
#ifndef INC_scope_B
#include "../ZUDIR/scope.b.c"
#endif
#ifndef INC_symbol_B
#include "../ZUDIR/symbol.b.c"
#endif
#ifndef INC_zimbufile_B
#include "../ZUDIR/zimbufile.b.c"
#endif
CWrite_C *CWrite_C__FNEW() {
  CWrite_C *THIS = Zalloc(sizeof(CWrite_C));
  THIS->VtargetLang = 1;
  return THIS;
}
char *CWrite_C__FthisName(CWrite_C *THIS, Zbool AinsideNew) {
  return CWrite_C__X__VnewThisName;
}
char *CWrite_C__FgetLangName(CWrite_C *THIS) {
  return "C";
}
CZimbuFile__X__CCodeSpecific *CWrite_C__FgetCS(CWrite_C *THIS, CZimbuFile *AzimbuFile) {
  return AzimbuFile->Vc;
}
Zbool CWrite_C__FtoplevelLines(CWrite_C *THIS, CZimbuFile *AzimbuFile) {
  return !(COutput__X__CFragmentHead__Fempty(AzimbuFile->Vc->Vheads->VmainLines));
}
void CWrite_C__FmainHead(CWrite_C *THIS, CSContext *Actx) {
  COutput__Fwrite(Actx->Vout, "int main(int argc, char **argv) {\n");
  MLibARG__FwriteMain(Actx->Vout);
  COutput__Fwrite(Actx->Vout, "  ZglobInit();\nreturn Fmain();\n}\nint Fmain() {\n");
}
void CWrite_C__FmainEnd(CWrite_C *THIS, CNode *AmainNode, CSContext *Actx) {
  CNode *Vnode;
  Vnode = AmainNode->Vn_left;
  if ((Vnode != NULL))
  {
    while ((Vnode->Vn_next != NULL))
    {
      Vnode = Vnode->Vn_next;
    }
  }
  if (((Vnode == NULL) || (Vnode->Vn_type != 71)))
  {
    COutput__Fwrite(Actx->Vout, "  return 0;\n");
  }
  COutput__Fwrite(Actx->Vout, "}\n");
}
void CWrite_C__FcopyC(CWrite_C *THIS, CNode *Anode, CSContext *Actx) {
  COutput__Fwrite(Actx->Vout, Anode->Vn_string);
}
void CWrite_C__FcopyJS(CWrite_C *THIS, CNode *Anode, CSContext *Actx) {
}
void CWrite_C__FwriteAlloc(CWrite_C *THIS, char *AtypeName, CSContext *Actx) {
  COutput__Fwrite(Actx->Vout, Zconcat(Zconcat("Zalloc(sizeof(", AtypeName), "))"));
  CScope__FaddUsedItem(Actx->Vscope, "alloc");
}
void CWrite_C__FwriteListAlloc(CWrite_C *THIS, CSContext *Actx) {
  CWrite_C__FwriteAlloc(THIS, "CListHead", Actx);
}
void CWrite_C__FwriteNewThis(CWrite_C *THIS, CSymbol *Asym, CSContext *Actx) {
  COutput__FwriteIndent(Actx->Vout, 1);
  COutput__Fwrite(Actx->Vout, Zconcat(Zconcat(Zconcat(Asym->VclassName, " *"), Actx->Vscope->VthisName), " = "));
  CWrite_C__FwriteAlloc(THIS, Asym->VclassName, Actx);
  COutput__Fwrite(Actx->Vout, ";\n");
}
void CWrite_C__FwriteNewReturn(CWrite_C *THIS, COutput *Aout) {
  COutput__FwriteIndent(Aout, 1);
  COutput__Fwrite(Aout, Zconcat(Zconcat("return ", CWrite_C__X__VnewThisName), ";\n"));
}
void CWrite_C__FwriteSymName(CWrite_C *THIS, CSymbol *Asym, CSContext *Actx) {
  if ((Asym->Vtype == 6))
  {
    COutput__Fwrite(Actx->Vout, "(*");
    if ((Asym->VreturnSymbol != NULL))
    {
      COutput__Fwrite(Actx->Vout, Asym->VreturnSymbol->VcName);
    }
    COutput__Fwrite(Actx->Vout, ")");
  }
else if ((Asym->VclassName != NULL))
  {
    COutput__Fwrite(Actx->Vout, Zconcat(Zconcat(Actx->Vscope->VthisName, "->"), Asym->VcName));
  }
else
  {
    COutput__Fwrite(Actx->Vout, Asym->VcName);
  }
}
CSymbol *CWrite_C__Fid(CWrite_C *THIS, CNode *Anode, CSContext *Actx, CSymbol *AdestSym) {
  CSymbol *Vsym;
  Vsym = Anode->Vn_symbol;
  if ((Vsym == NULL))
  {
    if (Actx->Vout->Vwriting)
    {
      CNode__Ferror(Anode, Zconcat("unknown symbol: ", Anode->Vn_string));
    }
  }
else
  {
    CWrite_C__FwriteSymName(THIS, Vsym, Actx);
  }
  return Anode->Vn_returnSymbol;
}
CSymbol *CWrite_C__FmethodReturnType(CWrite_C *THIS, CNode *Anode, CSymbol *Asym, CSContext *Actx) {
  CSymbol *VretSym;
  VretSym = NULL;
  if ((Anode->Vn_type == 33))
  {
    VretSym = Actx->Vscope->Vclass;
    COutput__Fwrite(Actx->Vout, Zconcat(VretSym->VclassName, " *"));
  }
else if ((Anode->Vn_type == 32))
  {
    VretSym = CSymbol__FNEW(11);
    COutput__Fwrite(Actx->Vout, "Zbool ");
  }
else if ((Anode->Vn_type == 34))
  {
    VretSym = MGenerate__FgenerateDeclType(Anode->Vn_returnType, CSContext__FcopyNoOut(Actx));
    if ((VretSym != NULL))
    {
      MGenerate__FgenType(VretSym, Anode->Vn_returnType, Actx->Vout);
    }
  }
else
  {
    COutput__Fwrite(Actx->Vout, "void ");
  }
  COutput__Fwrite(Actx->Vout, Zconcat(Asym->VcName, "("));
  if ((CScope__FisClassScope(Actx->Vscope) && (Zstrcmp(Asym->Vname, "NEW") != 0)))
  {
    COutput__Fwrite(Actx->Vout, Zconcat(Zconcat(Asym->VclassName, " *"), Actx->Vscope->VthisName));
    if ((Anode->Vn_left != NULL))
    {
      COutput__Fwrite(Actx->Vout, ", ");
    }
  }
  return VretSym;
}
void CWrite_C__FwriteMethodCall(CWrite_C *THIS, CSymbol *Afunc, Zbool AmoreArgs, CSContext *Actx) {
  if ((Afunc->VclassName != NULL))
  {
    COutput__Fwrite(Actx->Vout, Zconcat(Zconcat(Afunc->VcName, "("), Actx->Vscope->VthisName));
    if (AmoreArgs)
    {
      COutput__Fwrite(Actx->Vout, ", ");
    }
  }
else
  {
    COutput__Fwrite(Actx->Vout, Zconcat(Afunc->VcName, "("));
  }
}
void CWrite_C__FargWithType(CWrite_C *THIS, Zbool Afirst, CSymbol *AtypeSym, CNode *AtypeNode, char *AargName, CSContext *Actx) {
  if (!(Afirst))
  {
    COutput__Fwrite(Actx->Vout, ", ");
  }
  MGenerate__FgenType(AtypeSym, AtypeNode, Actx->Vout);
  COutput__Fwrite(Actx->Vout, AargName);
}
Zbool CWrite_C__FdoWriteDecl(CWrite_C *THIS) {
  return 1;
}
CSymbol *CWrite_C__Fsubscript(CWrite_C *THIS, CNode *Anode, CSContext *Actx, CSymbol *AdestSym) {
  CSymbol *Vsym;
  Vsym = MGenerate__FgenExpr(Anode->Vn_left, CSContext__FcopyNoOut(Actx), NULL);
  CSymbol *Vret;
  Vret = NULL;
  if ((Vsym != NULL))
  {
    if ((Vsym->Vtype == 13))
    {
      Vret = MListStuff__FgenerateSubscript(Vsym, Anode, 0, Actx, AdestSym);
    }
  else if ((Vsym->Vtype == 14))
    {
      Vret = MDictStuff__FgenerateSubscript(Vsym, Anode, 0, Actx, AdestSym);
    }
  else if ((Vsym->Vtype == 16))
    {
      MGenerate__FgenExpr(Anode->Vn_left, Actx, CSymbol__X__Varray);
      COutput__Fwrite(Actx->Vout, "[");
      MGenerate__FgenExpr(Anode->Vn_right, Actx, CSymbol__X__Vint);
      COutput__Fwrite(Actx->Vout, "]");
      if ((Vsym->VreturnSymbol != NULL))
      {
        Vret = CSymbol__Fcopy(Vsym->VreturnSymbol);
      }
    else if (Actx->Vout->Vwriting)
      {
        CNode__Ferror(Anode, "type of array item unknown");
      }
    }
  else if ((Vsym->Vtype == 9))
    {
      MGenerate__FgenExpr(Anode->Vn_left, Actx, CSymbol__X__Vstring);
      COutput__Fwrite(Actx->Vout, "[");
      MGenerate__FgenExpr(Anode->Vn_right, Actx, CSymbol__X__Vint);
      COutput__Fwrite(Actx->Vout, "]");
      Vret = CSymbol__X__Vint;
    }
  else if (Actx->Vout->Vwriting)
    {
      CNode__Ferror(Anode, Zconcat("type does not allow subscript: ", Zenum2string(CNode__X__EType, sizeof(CNode__X__EType) / sizeof(char *), Vsym->Vtype)));
    }
  }
else if (Actx->Vout->Vwriting)
  {
    MGenerate__FgenExpr(Anode->Vn_left, Actx, NULL);
  }
  return Vret;
}
void CWrite_C__FiobjectMember(CWrite_C *THIS, CSymbol *AobjSym, CSymbol *AitfSym, CNode *Anode, CSContext *Actx, CSymbol *AdestSym) {
  COutput__Fwrite(Actx->Vout, "(*(");
  MGenerate__FgenType(AobjSym, Anode, Actx->Vout);
  COutput__Fwrite(Actx->Vout, "*)(");
  MGenerate__FgenExpr(Anode->Vn_left, Actx, AdestSym);
  COutput__Fwrite(Actx->Vout, Zconcat(Zconcat("->ptr + ", MGenerate__FclassOffTableName(AitfSym->Vclass, AobjSym)), "["));
  MGenerate__FgenExpr(Anode->Vn_left, Actx, AdestSym);
  COutput__Fwrite(Actx->Vout, "->type]))");
  Anode->Vn_undefined = Anode->Vn_left->Vn_undefined;
}
void CWrite_C__FnumberOp(CWrite_C *THIS, CNode *Anode, CSContext *Actx) {
  COutput__Fwrite(Actx->Vout, "(");
  MGenerate__FgenExpr(Anode->Vn_left, Actx, CSymbol__X__Vint);
  char *Vop;
  Vop = NULL;
  switch (Anode->Vn_type)
  {
  case 73:
    {
      {
        Vop = " & ";
      }
        break;
    }
  case 74:
    {
      {
        Vop = " | ";
      }
        break;
    }
  case 75:
    {
      {
        Vop = " ^ ";
      }
        break;
    }
  case 89:
    {
      {
        Vop = " >> ";
      }
        break;
    }
  case 90:
    {
      {
        Vop = " << ";
      }
        break;
    }
  case 77:
    {
      {
        Vop = " * ";
      }
        break;
    }
  case 78:
    {
      {
        Vop = " / ";
      }
        break;
    }
  case 79:
    {
      {
        Vop = " % ";
      }
        break;
    }
  case 80:
    {
      {
        Vop = " + ";
      }
        break;
    }
  case 81:
    {
      {
        Vop = " - ";
      }
        break;
    }
  default:
    {
      {
        CNode__Ferror(Anode, "INTERNAL: numberOp not implemented");
      }
        break;
    }
  }
  COutput__Fwrite(Actx->Vout, Vop);
  MGenerate__FgenExpr(Anode->Vn_right, Actx, CSymbol__X__Vint);
  COutput__Fwrite(Actx->Vout, ")");
}
void CWrite_C__FconcatOp(CWrite_C *THIS, CNode *Anode, CSContext *Actx) {
  CScope__FaddUsedItem(Actx->Vscope, "concat");
  COutput__Fwrite(Actx->Vout, "Zconcat(");
  MGenerate__FgenExpr(Anode->Vn_left, Actx, CSymbol__X__Vstring);
  COutput__Fwrite(Actx->Vout, ", ");
  MGenerate__FgenExpr(Anode->Vn_right, Actx, CSymbol__X__Vstring);
  COutput__Fwrite(Actx->Vout, ")");
}
Zenum CWrite_C__FplusOp(CWrite_C *THIS, CNode *Anode, CSContext *Actx, Zenum AdestType) {
  CWrite_C__FnumberOp(THIS, Anode, Actx);
  return 7;
}
void CWrite_C__FincrdecrOp(CWrite_C *THIS, CNode *Anode, CSContext *Actx) {
  if ((Anode->Vn_type == 85))
  {
    COutput__Fwrite(Actx->Vout, "++");
  }
else if ((Anode->Vn_type == 86))
  {
    COutput__Fwrite(Actx->Vout, "--");
  }
  COutput__Fwrite(Actx->Vout, "(");
  MGenerate__FgenExpr(Anode->Vn_left, Actx, CSymbol__X__Vint);
  COutput__Fwrite(Actx->Vout, ")");
  if ((Anode->Vn_type == 87))
  {
    COutput__Fwrite(Actx->Vout, "++");
  }
else if ((Anode->Vn_type == 88))
  {
    COutput__Fwrite(Actx->Vout, "--");
  }
}
void CWrite_C__FbooleanOp(CWrite_C *THIS, CNode *Anode, CSContext *Actx) {
  COutput__Fwrite(Actx->Vout, "(");
  if ((Anode->Vn_nodeType == 2))
  {
    MGenerate__FgenExpr(Anode->Vn_left, Actx, CSymbol__X__Vnil);
    if (((Anode->Vn_type == 92) || (Anode->Vn_type == 98)))
    {
      COutput__Fwrite(Actx->Vout, " != ");
    }
  else
    {
      COutput__Fwrite(Actx->Vout, " == ");
    }
    MGenerate__FgenExpr(Anode->Vn_right, Actx, CSymbol__X__Vnil);
  }
else if ((Anode->Vn_nodeType == 9))
  {
    CScope__FaddUsedItem(Actx->Vscope, "strcmp");
    COutput__Fwrite(Actx->Vout, "Zstrcmp(");
    MGenerate__FgenExpr(Anode->Vn_left, Actx, CSymbol__X__Vstring);
    COutput__Fwrite(Actx->Vout, ", ");
    MGenerate__FgenExpr(Anode->Vn_right, Actx, CSymbol__X__Vstring);
    if ((Anode->Vn_type == 91))
    {
      COutput__Fwrite(Actx->Vout, ") == 0");
    }
  else
    {
      COutput__Fwrite(Actx->Vout, ") != 0");
    }
  }
else if ((Anode->Vn_nodeType == 24))
  {
    if ((Anode->Vn_symbol->Vtype == 24))
    {
      CNode *Vm;
      Vm = CNode__FNEW(107);
      Vm->Vn_left = CNode__FNEW(105);
      Vm->Vn_left->Vn_string = "EQUAL";
      Vm->Vn_left->Vn_left = Anode->Vn_left;
      Vm->Vn_left->Vn_start = Anode->Vn_start;
      Vm->Vn_right = Anode->Vn_right;
      MGenerate__FgenerateCall(Vm, Actx, CSymbol__X__Vbool, 1);
    }
  else
    {
      MGenerate__FgenExpr(Anode->Vn_left, Actx, Anode->Vn_symbol);
      if ((Anode->Vn_type == 91))
      {
        COutput__Fwrite(Actx->Vout, " == ");
      }
    else
      {
        COutput__Fwrite(Actx->Vout, " != ");
      }
      MGenerate__FgenExpr(Anode->Vn_right, Actx, Anode->Vn_symbol);
    }
  }
else
  {
    Zbool Verr;
    Verr = (MGenerate__FgenExpr(Anode->Vn_left, Actx, NULL) == NULL);
    Verr = (Verr || (MGenerate__FgenExpr(Anode->Vn_right, Actx, NULL) == NULL));
    if (!(Verr))
    {
      CNode__Ferror(Anode, "INTERNAL: booleanOp()");
    }
  }
  COutput__Fwrite(Actx->Vout, ")");
}
void CWrite_C__FcompareOp(CWrite_C *THIS, CNode *Anode, CSContext *Actx) {
  COutput__Fwrite(Actx->Vout, "(");
  MGenerate__FgenExpr(Anode->Vn_left, Actx, CSymbol__X__Vint);
  switch (Anode->Vn_type)
  {
  case 95:
    {
      {
        COutput__Fwrite(Actx->Vout, " < ");
      }
        break;
    }
  case 96:
    {
      {
        COutput__Fwrite(Actx->Vout, " <= ");
      }
        break;
    }
  case 93:
    {
      {
        COutput__Fwrite(Actx->Vout, " > ");
      }
        break;
    }
  case 94:
    {
      {
        COutput__Fwrite(Actx->Vout, " >= ");
      }
        break;
    }
  }
  MGenerate__FgenExpr(Anode->Vn_right, Actx, CSymbol__X__Vint);
  COutput__Fwrite(Actx->Vout, ")");
}
void CWrite_C__FandorOp(CWrite_C *THIS, CNode *Anode, CSContext *Actx) {
  COutput__Fwrite(Actx->Vout, "(");
  MGenerate__FgenExpr(Anode->Vn_left, Actx, CSymbol__X__Vbool);
  if ((Anode->Vn_type == 101))
  {
    COutput__Fwrite(Actx->Vout, " && ");
  }
else
  {
    COutput__Fwrite(Actx->Vout, " || ");
  }
  MGenerate__FgenExpr(Anode->Vn_right, Actx, CSymbol__X__Vbool);
  COutput__Fwrite(Actx->Vout, ")");
}
CSymbol *CWrite_C__Fparens(CWrite_C *THIS, CNode *Anode, CSContext *Actx, CSymbol *AdestSym) {
  COutput__Fwrite(Actx->Vout, "(");
  CSymbol *Vret;
  Vret = MGenerate__FgenExpr(Anode->Vn_left, Actx, AdestSym);
  COutput__Fwrite(Actx->Vout, ")");
  return Vret;
}
CSymbol *CWrite_C__FaltOp(CWrite_C *THIS, CNode *Anode, CSContext *Actx, CSymbol *AdestSym) {
  COutput__Fwrite(Actx->Vout, "(");
  MGenerate__FgenExpr(Anode->Vn_cond, Actx, CSymbol__X__Vbool);
  COutput__Fwrite(Actx->Vout, ") ? (");
  MGenerate__FgenExpr(Anode->Vn_left, Actx, AdestSym);
  COutput__Fwrite(Actx->Vout, ") : (");
  CSymbol *Vsymr;
  Vsymr = MGenerate__FgenExpr(Anode->Vn_right, Actx, AdestSym);
  COutput__Fwrite(Actx->Vout, ")");
  if ((Vsymr != NULL))
  {
    return CSymbol__Fcopy(Vsymr);
  }
  return NULL;
}
CSymbol *CWrite_C__FlistPart(CWrite_C *THIS, CNode *Anode, CSContext *Actx, CSymbol *AdestSym) {
  return MListStuff__FgenerateListPart_C(Anode, Actx, AdestSym);
}
void CWrite_C__FdictPart(CWrite_C *THIS, CNode *Anode, CSymbol *Aret, CSContext *Actx) {
  MDictStuff__FgenerateDictPart_C(Anode, Aret, Actx);
}
CSymbol *CWrite_C__Fexpr(CWrite_C *THIS, CNode *Anode, CSContext *Actx, CSymbol *AdestSym) {
  CSymbol *VretConverted;
  VretConverted = NULL;
  char *Vclose;
  Vclose = "";
  if (((AdestSym != NULL) && (AdestSym->Vtype != 0)))
  {
    switch (Anode->Vn_conversion)
    {
    case 1:
      {
        {
          CScope__FaddUsedItem(Actx->Vscope, "int2string");
          COutput__Fwrite(Actx->Vout, "Zint2string(");
          Vclose = ")";
          VretConverted = CSymbol__FNEW(9);
        }
          break;
      }
    case 2:
      {
        {
          CScope__FaddUsedItem(Actx->Vscope, "bool2string");
          COutput__Fwrite(Actx->Vout, "Zbool2string(");
          Vclose = ")";
          VretConverted = CSymbol__FNEW(9);
        }
          break;
      }
    case 3:
      {
        {
          CScope__FaddUsedItem(Actx->Vscope, "status2string");
          COutput__Fwrite(Actx->Vout, "Zstatus2string(");
          Vclose = ")";
          VretConverted = CSymbol__FNEW(9);
        }
          break;
      }
    case 4:
      {
        {
          CScope__FaddUsedItem(Actx->Vscope, "allocZoref");
          COutput__Fwrite(Actx->Vout, "ZallocZoref(");
          Zint Vidx;
          Vidx = CSymbol__FchildIndex(AdestSym->Vclass, Anode->Vn_ret_class);
          if ((Vidx < 0))
          {
            CNode__Ferror(Anode, "Class type mismatch");
          }
          Vclose = Zconcat(Zconcat(", ", Zint2string(Vidx)), ")");
          VretConverted = AdestSym;
        }
          break;
      }
    }
  }
  CSymbol *Vret;
  Vret = MGenerate__FgenExprChecked(Anode, Actx, AdestSym);
  COutput__Fwrite(Actx->Vout, Vclose);
  if ((VretConverted != NULL))
  {
    return VretConverted;
  }
  return Vret;
}
char *CWrite_C__FwriteImport(CWrite_C *THIS, CZimbuFile *Aimport, COutput__X__CGroup *AdummyOuts, CScope *Ascope) {
  
#if defined(__MINGW32__) || defined(_MSC_VER)
_mkdir(Aimport->VoutDir)
#else
mkdir(Aimport->VoutDir, 0777)
#endif
;
  char *Vroot;
  Vroot = Zconcat(Zconcat(Aimport->VoutDir, "/"), Aimport->VrootName);
  char *VdefRoot;
  VdefRoot = Zconcat("INC_", Aimport->VrootName);
  if (!(COutput__X__CFragmentHead__Fempty(Aimport->Vc->Vheads->Vinits)))
  {
    COutput__X__CFragmentHead__Fadd(Aimport->Vc->Vheads->Vdeclares, Zconcat(Zconcat("void ", Aimport->VinitFunc), "();\n"));
    COutput__X__CFragmentHead__Fadd(Aimport->Vc->Vheads->VfuncBodies, Zconcat(Zconcat("void ", Aimport->VinitFunc), "() {\n"));
    COutput__X__CFragmentHead__Fadd(Aimport->Vc->Vheads->VfuncBodies, " static int done = 0;\n");
    COutput__X__CFragmentHead__Fadd(Aimport->Vc->Vheads->VfuncBodies, " if (!done) {\n");
    COutput__X__CFragmentHead__Fadd(Aimport->Vc->Vheads->VfuncBodies, "  done = 1;\n");
    COutput__X__CFragmentHead__Fappend__1(Aimport->Vc->Vheads->VfuncBodies, Aimport->Vc->Vheads->Vinits);
    COutput__X__CFragmentHead__Fadd(Aimport->Vc->Vheads->VfuncBodies, " }\n");
    COutput__X__CFragmentHead__Fadd(Aimport->Vc->Vheads->VfuncBodies, "}\n");
  }
  CWrite_C__FwriteImportFile(THIS, Zconcat(Vroot, ".t.h"), Aimport->Vc->Vheads->Vtypedefs, "TYPEDEFS", Zconcat(VdefRoot, "_T"));
  CWrite_C__FwriteImportFile(THIS, Zconcat(Vroot, ".s.h"), Aimport->Vc->Vheads->Vstructs, "STRUCTS", Zconcat(VdefRoot, "_S"));
  CWrite_C__FwriteImportFile(THIS, Zconcat(Vroot, ".d.h"), Aimport->Vc->Vheads->Vdeclares, "DECLARE FUNCTIONS AND GLOBALS", Zconcat(VdefRoot, "_D"));
  CWrite_C__FwriteImportFile(THIS, Zconcat(Vroot, ".b.c"), Aimport->Vc->Vheads->VfuncBodies, "FUNCTION BODIES", Zconcat(VdefRoot, "_B"));
  return Zconcat(Vroot, ".b.c");
}
void CWrite_C__FwriteImportFile(CWrite_C *THIS, char *Afname, COutput__X__CFragmentHead *Ahead, char *Aheader, char *Adef) {
  FILE *Vfd;
  Vfd = fopen(Afname, "w");
  if ((Vfd == NULL))
  {
    (fputs(Zconcat("ERROR: Cannot open file for writing: ", Afname), stdout) | fputc('\n', stdout));
    exit(1);
  }
  fputs(Zconcat(Zconcat("#define ", Adef), " 1\n"), Vfd);
  fputs(Zconcat(Zconcat("/*\n * ", Aheader), "\n */\n"), Vfd);
  COutput__X__CFragmentHead__Fwrite__1(Ahead, Vfd);
  fclose(Vfd);
}
void CWrite_C__FwriteIncludeImport(CWrite_C *THIS, CZimbuFile *Aimport, COutput__X__CGroup *AmyOuts, CScope *Ascope) {
  char *Vpre;
  Vpre = Zconcat("#ifndef INC_", Aimport->VrootName);
  char *Vinc;
  Vinc = "#include \"";
  if ((Zstrcmp(Ascope->VimportIndent, "") != 0))
  {
    Vinc = Zconcat(Vinc, "../");
  }
  char *Vdir;
  Vdir = Ascope->VdirName;
  while (((Vdir != NULL) && (Zstrcmp(Vdir, "") != 0)))
  {
    Vinc = Zconcat(Vinc, "../");
    Zint Vslash;
    Vslash = ZStringIndex(Vdir, 47);
    if ((Vslash < 0))
    {
      break;
    }
    Vdir = ZStringByteSlice(Vdir, (Vslash + 1), -(1));
  }
  Vinc = Zconcat(Vinc, Zconcat(Aimport->VoutDir, "/"));
  Vinc = Zconcat(Vinc, Aimport->VrootName);
  COutput__Fwrite(AmyOuts->VtypeOut, Zconcat(Vpre, "_T\n"));
  COutput__Fwrite(AmyOuts->VtypeOut, Zconcat(Vinc, ".t.h\"\n#endif\n"));
  COutput__Fwrite(AmyOuts->VstructOut, Zconcat(Vpre, "_S\n"));
  COutput__Fwrite(AmyOuts->VstructOut, Zconcat(Vinc, ".s.h\"\n#endif\n"));
  COutput__Fwrite(AmyOuts->VdeclOut, Zconcat(Vpre, "_D\n"));
  COutput__Fwrite(AmyOuts->VdeclOut, Zconcat(Vinc, ".d.h\"\n#endif\n"));
  COutput__Fwrite(AmyOuts->VbodyOut, Zconcat(Vpre, "_B\n"));
  COutput__Fwrite(AmyOuts->VbodyOut, Zconcat(Vinc, ".b.c\"\n#endif\n"));
  if (!(COutput__X__CFragmentHead__Fempty(Aimport->Vc->Vheads->Vinits)))
  {
    COutput__Fwrite(AmyOuts->VinitOut, Zconcat(Zconcat("  ", Aimport->VinitFunc), "();\n"));
  }
}
Zbool CWrite_C__FneedWrite(CWrite_C *THIS, CZimbuFile *AzimbuFile) {
  if ((AzimbuFile->Vc->VstartedWrite != NULL))
  {
    return 0;
  }
  AzimbuFile->Vc->VstartedWrite = "yes";
  return 1;
}
void CWrite_C__FwriteClassDef(CWrite_C *THIS, char *Aname, COutput *AtypeOut) {
  COutput__Fwrite(AtypeOut, Zconcat(Zconcat(Zconcat(Zconcat("typedef struct ", Aname), "__S "), Aname), ";\n"));
}
void CWrite_C__FwriteClassDecl(CWrite_C *THIS, CSymbol *AclassSym, COutput__X__CGroup *Aouts, COutput *AstructOut) {
  char *Vname;
  Vname = AclassSym->VcName;
  COutput__Fwrite(Aouts->VstructOut, Zconcat(Zconcat("struct ", Vname), "__S {\n"));
  COutput__Fappend(Aouts->VstructOut, AstructOut);
  COutput__Fwrite(Aouts->VstructOut, "};\n");
}
void CWrite_C__FdefaultInit(CWrite_C *THIS, CSymbol *Asym, COutput *Aout) {
  switch (Asym->Vtype)
  {
  case 21:
  case 25:
  case 9:
  case 13:
  case 14:
    {
      {
        COutput__Fwrite(Aout, "NULL");
      }
        break;
    }
  default:
    {
      {
        COutput__Fwrite(Aout, "0");
      }
        break;
    }
  }
}
void CWrite_C__Fnil(CWrite_C *THIS, CSContext *Actx) {
  COutput__Fwrite(Actx->Vout, "NULL");
}
void CWrite_C__Fmember(CWrite_C *THIS, char *Aname, COutput *Aout) {
  COutput__Fwrite(Aout, Zconcat("->", Aname));
}
void CWrite_C__Fvardecl(CWrite_C *THIS, CSContext *Actx, COutput *Aout) {
}
void CWrite_C__Fvartype(CWrite_C *THIS, CSymbol *Asym, CNode *Anode, CScope *Ascope, COutput *Aout) {
  switch (Asym->Vtype)
  {
  case 21:
  case 24:
    {
      {
        if ((((Asym->Vattributes) & 1)))
        {
          CNode__Ferror(Anode, Zconcat("Cannot use abstract class ", Asym->Vname));
        }
        COutput__Fwrite(Aout, Zconcat(Asym->VclassName, " *"));
        return ;
      }
        break;
    }
  case 25:
    {
      {
        COutput__Fwrite(Aout, "Zoref *");
        return ;
      }
        break;
    }
  case 13:
    {
      {
        COutput__Fwrite(Aout, "CListHead *");
        return ;
      }
        break;
    }
  case 14:
    {
      {
        COutput__Fwrite(Aout, "CDictHead *");
        return ;
      }
        break;
    }
  case 37:
    {
      {
        COutput__Fwrite(Aout, "void *");
        return ;
      }
        break;
    }
  case 36:
    {
      {
        COutput__Fwrite(Aout, "void *");
        return ;
      }
        break;
    }
  case 9:
    {
      {
        COutput__Fwrite(Aout, "char *");
        return ;
      }
        break;
    }
  case 6:
    {
      {
        CWrite_C__Fvartype(THIS, Asym->VreturnSymbol, Anode, Ascope, Aout);
        COutput__Fwrite(Aout, " *");
        return ;
      }
        break;
    }
  case 11:
    {
      {
        COutput__Fwrite(Aout, "Zbool ");
        return ;
      }
        break;
    }
  case 12:
    {
      {
        COutput__Fwrite(Aout, "Zstatus ");
        return ;
      }
        break;
    }
  case 26:
    {
      {
        COutput__Fwrite(Aout, "Zbits ");
        return ;
      }
        break;
    }
  case 27:
    {
      {
        COutput__Fwrite(Aout, "Zbbits ");
        return ;
      }
        break;
    }
  case 29:
    {
      {
        COutput__Fwrite(Aout, "Zenum ");
        return ;
      }
        break;
    }
  case 7:
    {
      {
        COutput__Fwrite(Aout, "Zint ");
        return ;
      }
        break;
    }
  default:
    {
      {
        if (Aout->Vwriting)
        {
          CNode__Ferror(Anode, Zconcat("Declaration of unknown type ", Zenum2string(CNode__X__EType, sizeof(CNode__X__EType) / sizeof(char *), Asym->Vtype)));
        }
      }
        break;
    }
  }
}
void CWrite_C__FforStart(CWrite_C *THIS, COutput *Aout) {
  COutput__Fwrite(Aout, "Zfor_T *Zf = ZforNew(");
}
void CWrite_C__FforLoop(CWrite_C *THIS, CSymbol *AvarSym, COutput *Aout) {
  char *VtypeName;
  VtypeName = (CSymbol__FisPointerType(AvarSym)) ? ("Ptr") : ("Int");
  char *Vcast;
  Vcast = (CSymbol__FisPointerType(AvarSym)) ? ("(char **)") : ("");
  COutput__Fwrite(Aout, Zconcat(Zconcat(Zconcat(Zconcat(Zconcat(Zconcat("for (ZforGet", VtypeName), "(Zf, "), Vcast), "&"), AvarSym->VcName), "); "));
  COutput__Fwrite(Aout, "ZforCont(Zf); ");
  COutput__Fwrite(Aout, Zconcat(Zconcat(Zconcat(Zconcat(Zconcat(Zconcat("ZforNext", VtypeName), "(Zf, "), Vcast), "&"), AvarSym->VcName), ")) {\n"));
}
void CWrite_C__X__FwriteFile(COutput__X__CHeads *Aheads, CScope *AtopScope, FILE *AoutFile) {
  COutput__X__CFragmentHead__Fadd(Aheads->Vdeclares, "void ZglobInit();\n");
  COutput__X__CFragmentHead__Finsert(Aheads->Vinits, "void ZglobInit() {\n");
  COutput__X__CFragmentHead__Fadd(Aheads->Vinits, "}\n\n");
  CWrite_C__X__FwriteIncludes(AtopScope, AoutFile);
  fputs("/*\n * TYPEDEFS\n */\n", AoutFile);
  CWrite_C__X__FwriteTypedefs(AtopScope, AoutFile);
  COutput__X__CFragmentHead__Fwrite__1(Aheads->Vtypedefs, AoutFile);
  fputs("/*\n * STRUCTS\n */\n", AoutFile);
  CWrite_C__X__FwriteDecl(AtopScope, AoutFile);
  COutput__X__CFragmentHead__Fwrite__1(Aheads->Vstructs, AoutFile);
  fputs("/*\n * DECLARE FUNCTIONS AND GLOBALS\n */\n", AoutFile);
  CWrite_C__X__FwriteBodies(AtopScope, AoutFile);
  COutput__X__CFragmentHead__Fwrite__1(Aheads->Vdeclares, AoutFile);
  fputs("/*\n * FUNCTION BODIES\n */\n", AoutFile);
  COutput__X__CFragmentHead__Fwrite__1(Aheads->VfuncBodies, AoutFile);
  fputs("/*\n * INIT GLOBALS\n */\n", AoutFile);
  COutput__X__CFragmentHead__Fwrite__1(Aheads->Vinits, AoutFile);
  fputs("/*\n * MAIN\n */\n", AoutFile);
  COutput__X__CFragmentHead__Fwrite__1(Aheads->VmainLines, AoutFile);
}
void CWrite_C__X__FwriteIncludes(CScope *AtopScope, FILE *Afd) {
  if ((((((((((ZDictHas(AtopScope->VusedItems, 0, "strcmp") || ZDictHas(AtopScope->VusedItems, 0, "concat")) || ZDictHas(AtopScope->VusedItems, 0, "stringSlice")) || ZDictHas(AtopScope->VusedItems, 0, "stringByteSlice")) || ZDictHas(AtopScope->VusedItems, 0, "garray")) || ZDictHas(AtopScope->VusedItems, 0, "stringToLower")) || ZDictHas(AtopScope->VusedItems, 0, "stringToUpper")) || ZDictHas(AtopScope->VusedItems, 0, "stringFind")) || ZDictHas(AtopScope->VusedItems, 0, "stringStartsWith")) || ZDictHas(AtopScope->VusedItems, 0, "stringEndsWith")))
  {
    fputs("#include <string.h>\n", Afd);
  }
  fputs("#include <stdlib.h>\n", Afd);
  if (ZDictHas(AtopScope->VusedItems, 0, "unistd.h"))
  {
    fputs("#include <unistd.h>\n", Afd);
  }
  if (ZDictHas(AtopScope->VusedItems, 0, "sys/types.h"))
  {
    fputs("#include <sys/types.h>\n", Afd);
  }
  if (ZDictHas(AtopScope->VusedItems, 0, "fcntl.h"))
  {
    fputs("#include <fcntl.h>\n", Afd);
  }
  if (ZDictHas(AtopScope->VusedItems, 0, "sys/stat.h"))
  {
    fputs("#include <sys/stat.h>\n", Afd);
  }
  if ((ZDictHas(AtopScope->VusedItems, 0, "stringToLower") || ZDictHas(AtopScope->VusedItems, 0, "stringToUpper")))
  {
    fputs("#include <ctype.h>\n", Afd);
  }
  fputs("#include <stdio.h>\n", Afd);
  if (ZDictHas(AtopScope->VusedItems, 0, "strings.h"))
  {
    fputs("#include <strings.h>\n", Afd);
  }
  if (ZDictHas(AtopScope->VusedItems, 0, "time.h"))
  {
    fputs("#include <time.h>\n", Afd);
  }
  fputs("\n", Afd);
  {
    Zfor_T *Zf = ZforNew(CWrite_C__X__VincludeWriters, 2);
    void *Vp;
    for (ZforGetPtr(Zf, (char **)&Vp); ZforCont(Zf); ZforNextPtr(Zf, (char **)&Vp)) {
      ((void (*)(CScope *, FILE *))Vp)(AtopScope, Afd);
    }
  }
}
void CWrite_C__X__FwriteTypedefs(CScope *AtopScope, FILE *Afd) {
  fputs("#ifdef __MINGW32__\n", Afd);
  fputs("typedef long Zint;\n", Afd);
  fputs("typedef long Zbbits;\n", Afd);
  fputs("#else\n", Afd);
  fputs(Zconcat(Zconcat("typedef ", MConfig__Vint64name), " Zint;\n"), Afd);
  fputs(Zconcat(Zconcat("typedef ", MConfig__Vint64name), " Zbbits;\n"), Afd);
  fputs("#endif\n", Afd);
  fputs("typedef int Zbits;\n", Afd);
  fputs("typedef int Zbool;\n", Afd);
  fputs("typedef int Zstatus;\n", Afd);
  fputs("typedef int Zenum;\n", Afd);
  fputs("typedef struct Zoref__S Zoref;\n", Afd);
  if (ZDictHas(AtopScope->VusedItems, 0, "garray"))
  {
    fputs("typedef struct garray__S garray_T;\n", Afd);
  }
  if (ZDictHas(AtopScope->VusedItems, 0, "for"))
  {
    fputs("typedef struct Zfor__S Zfor_T;\n", Afd);
  }
  MListStuff__FwriteTypedefs_C(AtopScope, Afd);
  MDictStuff__FwriteTypedefs(AtopScope, Afd);
  {
    Zfor_T *Zf = ZforNew(CWrite_C__X__VtypedefWriters, 2);
    void *Vp;
    for (ZforGetPtr(Zf, (char **)&Vp); ZforCont(Zf); ZforNextPtr(Zf, (char **)&Vp)) {
      ((void (*)(CScope *, FILE *))Vp)(AtopScope, Afd);
    }
  }
  fputs("\n", Afd);
}
void CWrite_C__X__FwriteDecl(CScope *AtopScope, FILE *Afd) {
  fputs("#define MIO__Veof EOF\n", Afd);
  fputs("\n", Afd);
  if (ZDictHas(AtopScope->VusedItems, 0, "dummy"))
  {
    fputs("struct {} dummy;\n", Afd);
    fputs("\n", Afd);
  }
  if ((((ZDictHas(AtopScope->VusedItems, 0, "list") || MDictStuff__VuseDict) || ZDictHas(AtopScope->VusedItems, 0, "alloc")) || ZDictHas(AtopScope->VusedItems, 0, "for")))
  {
    fputs("\nvoid *Zalloc(size_t size) {\n  return calloc(1, size);\n}\n", Afd);
  }
  if ((ZDictHas(AtopScope->VusedItems, 0, "list") || MDictStuff__VuseDict))
  {
    fputs("\nvoid Zerror(char *msg) {\n  fprintf(stderr, \"ERROR: %s\\n\", msg);\n}\n", Afd);
  }
  fputs("\nstruct Zoref__S {\n  char *ptr;\n  int  type;\n};\n", Afd);
  if (ZDictHas(AtopScope->VusedItems, 0, "garray"))
  {
    fputs("\nstruct garray__S {\n  char *data;\n  int  used;\n  int  len;\n};\n", Afd);
  }
  if (ZDictHas(AtopScope->VusedItems, 0, "for"))
  {
    fputs("\nstruct Zfor__S {\n  int type;\n  CListItem *listItem;\n};\n", Afd);
  }
  MListStuff__FwriteDecl_C(AtopScope, Afd);
  MDictStuff__FwriteDecl(AtopScope, Afd);
  {
    Zfor_T *Zf = ZforNew(CWrite_C__X__VdeclWriters, 2);
    void *Vp;
    for (ZforGetPtr(Zf, (char **)&Vp); ZforCont(Zf); ZforNextPtr(Zf, (char **)&Vp)) {
      ((void (*)(CScope *, FILE *))Vp)(AtopScope, Afd);
    }
  }
}
void CWrite_C__X__FwriteBodies(CScope *AtopScope, FILE *Afd) {
  if (ZDictHas(AtopScope->VusedItems, 0, "allocZoref"))
  {
    fputs("\nZoref *ZallocZoref(void *ptr, Zint type) {\n  Zoref *p = Zalloc(sizeof(Zoref));\n  p->ptr = ptr;\n  p->type = type;\n  return p;\n}\n", Afd);
  }
  if (ZDictHas(AtopScope->VusedItems, 0, "garray"))
  {
    fputs("\nga_append(garray_T *ga, char *str) {\n  int len = strlen(str);\n  if (ga->data == NULL || ga->used + len >= ga->len) {\n    int newLen = ga->used + len + 200;\n    char *newData = malloc(newLen);\n    if (ga->data != NULL) {\n      memmove(newData, ga->data, ga->used);\n      free(ga->data);\n    }\n    ga->data = newData;\n    ga->len = newLen;\n  }\n  strcpy(ga->data + ga->used, str);\n  ga->used += len;\n}\n", Afd);
  }
  if (ZDictHas(AtopScope->VusedItems, 0, "strcmp"))
  {
    fputs("\nint Zstrcmp(char *s1, char *s2) {\n  if (s1 == NULL || s2 == NULL) {\n    if (s1 == NULL && s2 == NULL)\n      return 0;\n    if (s1 == NULL)\n      return -1;\n    return 1;\n  }\n  return strcmp(s1, s2);\n}\n", Afd);
  }
  if (ZDictHas(AtopScope->VusedItems, 0, "concat"))
  {
    fputs("\nchar *Zconcat(char *s1, char *s2) {\n  char *ss1 = s1 == NULL ? \"NULL\" : s1;\n  char *ss2 = s2 == NULL ? \"NULL\" : s2;\n  char *p = malloc(strlen(ss1) + strlen(ss2) + 1);\n  strcpy(p, ss1);\n  strcat(p, ss2);\n  return p;\n}\n", Afd);
  }
  if (ZDictHas(AtopScope->VusedItems, 0, "bool2string"))
  {
    fputs("\nchar *Zbool2string(Zbool n) {\n  return n == 0 ? \"FALSE\" : \"TRUE\";\n}\n", Afd);
  }
  if (ZDictHas(AtopScope->VusedItems, 0, "status2string"))
  {
    fputs("\nchar *Zstatus2string(Zstatus n) {\n  return n == 0 ? \"FAIL\" : \"OK\";\n}\n", Afd);
  }
  if (ZDictHas(AtopScope->VusedItems, 0, "int2string"))
  {
    fputs("\nchar *Zint2string(Zint n) {\n  char *p = malloc(30);\n  sprintf(p, \"%lld\", n);\n  return p;\n}\n", Afd);
  }
  if (ZDictHas(AtopScope->VusedItems, 0, "fixSign"))
  {
    fputs("\nZint ZFixSign(Zint n, Zint mask) {\n  if (n & mask)\n    return n | mask;\n  return n;\n}\n", Afd);
  }
  if (ZDictHas(AtopScope->VusedItems, 0, "tochar"))
  {
    fputs("\nchar *Ztochar(Zint n) {\n  char *p = malloc(2);\n  p[0] = n; p[1] = 0;\n  return p;\n}\n", Afd);
  }
  if (ZDictHas(AtopScope->VusedItems, 0, "enum2string"))
  {
    fputs("\nchar *Zenum2string(char **names, size_t nm, Zenum n) {\n", Afd);
    fputs("  return (n < 0 || n >= nm) ? \"INVALID\" : names[n];\n", Afd);
    fputs("}\n", Afd);
  }
  if (ZDictHas(AtopScope->VusedItems, 0, "stringSlice"))
  {
    fputs("\nchar *ZStringSlice(char *s, Zint start, Zint end) {\n  char *ss = (s == NULL) ? \"NULL\" : s;\n  char *r;\n  Zint l = (Zint)strlen(ss);\n  Zint is = start < 0 ? l + start : start;\n  Zint ie = end < 0 ? l + end : end;\n  if (is < 0) is = 0;\n  if (is > l) is = l;\n  if (ie < is - 1) ie = is - 1;\n  if (ie >= l) ie = l - 1;\n  l = ie - is + 1;\n  r = malloc(l + 1);\n  strncpy(r, ss + is, l);\n  r[l] = 0;\n  return r;\n}\n", Afd);
  }
  if (ZDictHas(AtopScope->VusedItems, 0, "stringByteSlice"))
  {
    fputs("\nchar *ZStringByteSlice(char *s, Zint start, Zint end) {\n  char *ss = (s == NULL) ? \"NULL\" : s;\n  char *r;\n  Zint l = (Zint)strlen(ss);\n  Zint is = start < 0 ? l + start : start;\n  Zint ie = end < 0 ? l + end : end;\n  if (is < 0) is = 0;\n  if (is > l) is = l;\n  if (ie < is - 1) ie = is - 1;\n  if (ie >= l) ie = l - 1;\n  l = ie - is + 1;\n  r = malloc(l + 1);\n  strncpy(r, ss + is, l);\n  r[l] = 0;\n  return r;\n}\n", Afd);
  }
  if (ZDictHas(AtopScope->VusedItems, 0, "stringToInt"))
  {
    fputs("\nZint ZStringToInt(char *s) {\n  Zint r;\n  if (s == NULL) return 0;\n  sscanf(s, \"%lld\", &r);\n  return r;\n}\n", Afd);
  }
  if (ZDictHas(AtopScope->VusedItems, 0, "stringToLower"))
  {
    fputs("\nchar *ZStringToLower(char *ss) {\n  char *r;\n  char *s;\n  char *d;\n  if (ss == NULL)\n    return NULL;\n  r = (char *)Zalloc(strlen(ss) + 1);\n  s = ss;\n  d = r;\n  while (*s != 0)\n    *d++ = tolower(*s++);\n  *d = 0;\n  return r;\n}\n", Afd);
  }
  if (ZDictHas(AtopScope->VusedItems, 0, "stringToUpper"))
  {
    fputs("\nchar *ZStringToUpper(char *ss) {\n  char *r;\n  char *s;\n  char *d;\n  if (ss == NULL)\n    return NULL;\n  r = (char *)Zalloc(strlen(ss) + 1);\n  s = ss;\n  d = r;\n  while (*s != 0)\n    *d++ = toupper(*s++);\n  *d = 0;\n  return r;\n}\n", Afd);
  }
  if (ZDictHas(AtopScope->VusedItems, 0, "stringFind"))
  {
    fputs("\nZint ZStringFind(char *big, char *small) {\n  char *r;\n  if (big == NULL || small == NULL)\n    return -1;\n  r = strstr(big, small);\n  if (r == NULL)\n    return -1;\n  return r - big;\n}\n", Afd);
  }
  if (ZDictHas(AtopScope->VusedItems, 0, "stringStartsWith"))
  {
    fputs("\nZbool ZStringStartsWith(char *big, char *small) {\n  int i;\n  if (big == NULL || small == NULL)\n    return 0;\n  for (i = 0; small[i] != 0; ++i) {\n    if (big[i] != small[i])\n      return 0;\n  }\n  return 1;\n}\n", Afd);
  }
  if (ZDictHas(AtopScope->VusedItems, 0, "stringEndsWith"))
  {
    fputs("\nZbool ZStringEndsWith(char *big, char *small) {\n  int i, j;\n  if (big == NULL || small == NULL)\n    return 0;\n  i = strlen(big) - 1;\n  j = strlen(small) - 1;\n  while (1) {\n    if (j < 0)\n      return 1;\n    if (i < 0)\n      return 0;\n    if (big[i] != small[j])\n      return 0;\n    --i;\n    --j;\n  }\n}\n", Afd);
  }
  if (ZDictHas(AtopScope->VusedItems, 0, "stringBinToInt"))
  {
    fputs("\nZint ZStringBinToInt(char *s) {\n  Zint r = 0;\n  if (s != NULL) {\n    char *p;\n    for (p = s; *p != 0; ++p) {\n      if (*p == '0')\n        r <<= 1;\n      else if (*p == '1')\n        r = (r << 1) + 1;\n      else\n        break;\n    }\n  }\n  return r;\n}\n", Afd);
  }
  if (ZDictHas(AtopScope->VusedItems, 0, "stringHexToInt"))
  {
    fputs("\nZint ZStringHexToInt(char *s) {\n  Zint r;\n  if (s == NULL) return 0;\n  sscanf(s, \"%llx\", &r);\n  return r;\n}\n", Afd);
  }
  if (ZDictHas(AtopScope->VusedItems, 0, "stringQuotedToInt"))
  {
    fputs("\nZint ZStringQuotedToInt(char *s) {\n  Zint r = 0;\n  if (s != NULL) {\n    char *p;\n    for (p = s; *p != 0; ++p) {\n      if (*p >= '0' && *p <= '9')\n        r = r * 10 + (*p - '0');\n      else if (*p != '\\'')\n        break;\n    }\n  }\n  return r;\n}\n", Afd);
  }
  if (ZDictHas(AtopScope->VusedItems, 0, "stringQuotedBinToInt"))
  {
    fputs("\nZint ZStringQuotedBinToInt(char *s) {\n  Zint r = 0;\n  if (s != NULL) {\n    char *p;\n    for (p = s; *p != 0; ++p) {\n      if (*p == '0')\n        r <<= 1;\n      else if (*p == '1')\n        r = (r << 1) + 1;\n      else if (*p != '\\'')\n        break;\n    }\n  }\n  return r;\n}\n", Afd);
  }
  if (ZDictHas(AtopScope->VusedItems, 0, "stringQuotedHexToInt"))
  {
    fputs("\nZint ZStringQuotedHexToInt(char *s) {\n  Zint r = 0;\n  if (s != NULL) {\n    char *p;\n    for (p = s; *p != 0; ++p) {\n      if (*p >= '0' && *p <= '9')\n        r = r * 16 + (*p - '0');\n      else if (*p >= 'a' && *p <= 'f')\n        r = r * 16 + (*p - 'a' + 10);\n      else if (*p >= 'A' && *p <= 'F')\n        r = r * 16 + (*p - 'A' + 10);\n      else if (*p != '\\'')\n        break;\n    }\n  }\n  return r;\n}\n", Afd);
  }
  if (ZDictHas(AtopScope->VusedItems, 0, "stringIndex"))
  {
    fputs("\nZint ZStringIndex(char *s, Zint c) {\n  unsigned char *p;\n  if (s == NULL) return -1;\n  for (p = (unsigned char *)s; *p != 0; ++p) {\n    if (*p == c) return (Zint)((char *)p - s);\n  }\n  return -1;\n}\n", Afd);
  }
  if (ZDictHas(AtopScope->VusedItems, 0, "stringRindex"))
  {
    fputs("\nZint ZStringRindex(char *s, Zint c) {\n  unsigned char *p;\n  unsigned char *found;\n  if (s == NULL) return -1;\n  found = NULL;\n  for (p = (unsigned char *)s; *p != 0; ++p) {\n    if (*p == c) found = p;\n  }\n  if (found == NULL) return -1;\n  return (int)((char *)found - s);\n}\n", Afd);
  }
  MListStuff__FwriteBody_C(AtopScope, Afd);
  MDictStuff__FwriteBody(AtopScope, Afd);
  if (ZDictHas(AtopScope->VusedItems, 0, "for"))
  {
    fputs("\nZfor_T *ZforNew(void *p, int type) {\n  Zfor_T *s = Zalloc(sizeof(Zfor_T));\n  s->type = type;\n  if (type == 2)\n    s->listItem = ((CListHead *)p)->first;\n  return s;\n}\nvoid ZforGetPtr(Zfor_T *s, char **p) {\n  if (s->type == 2 && s->listItem != NULL)\n    *p = s->listItem->Pval;\n  else\n    *p = NULL;\n}\nvoid ZforGetInt(Zfor_T *s, Zint *p) {\n  if (s->type == 2 && s->listItem != NULL)\n    *p = s->listItem->Ival;\n  else\n    *p = 0;\n}\nint ZforCont(Zfor_T *s) {\n  if (s->type == 2)\n    return s->listItem != NULL;\n  return 0;\n}\nvoid ZforNextPtr(Zfor_T *s, char **p) {\n  if (s->type == 2 && s->listItem != NULL)\n    s->listItem = s->listItem->next;\n  ZforGetPtr(s, p);\n}\nvoid ZforNextInt(Zfor_T *s, Zint *p) {\n  if (s->type == 2 && s->listItem != NULL)\n    s->listItem = s->listItem->next;\n  ZforGetInt(s, p);\n}\n", Afd);
  }
  {
    Zfor_T *Zf = ZforNew(CWrite_C__X__VbodyWriters, 2);
    void *Vp;
    for (ZforGetPtr(Zf, (char **)&Vp); ZforCont(Zf); ZforNextPtr(Zf, (char **)&Vp)) {
      ((void (*)(CScope *, FILE *))Vp)(AtopScope, Afd);
    }
  }
}
void Iwrite_c() {
 static int done = 0;
 if (!done) {
  done = 1;
  Iconfig();
  Idictstuff();
  Igenerate();
  Ilibarg();
  Iliststuff();
  Ioutput();
  Iresolve();
  Iscontext();
  Iscope();
  Isymbol();
  Izimbufile();
  CWrite_C__X__VnewThisName = "THIS";
  CWrite_C__X__VincludeWriters = Zalloc(sizeof(CListHead));
  CWrite_C__X__VtypedefWriters = Zalloc(sizeof(CListHead));
  CWrite_C__X__VdeclWriters = Zalloc(sizeof(CListHead));
  CWrite_C__X__VbodyWriters = Zalloc(sizeof(CListHead));
 }
}
