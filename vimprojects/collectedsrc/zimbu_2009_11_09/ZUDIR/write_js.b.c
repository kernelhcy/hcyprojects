#define INC_write_js_B 1
/*
 * FUNCTION BODIES
 */
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
CWrite_JS *CWrite_JS__FNEW() {
  CWrite_JS *THIS = Zalloc(sizeof(CWrite_JS));
  THIS->VtargetLang = 2;
  return THIS;
}
char *CWrite_JS__FgetLangName(CWrite_JS *THIS) {
  return "JS";
}
char *CWrite_JS__FthisName(CWrite_JS *THIS, Zbool AinsideNew) {
  if (AinsideNew)
  {
    return CWrite_JS__X__VnewThisName;
  }
  return "this";
}
CZimbuFile__X__CCodeSpecific *CWrite_JS__FgetCS(CWrite_JS *THIS, CZimbuFile *AzimbuFile) {
  return AzimbuFile->Vjs;
}
void CWrite_JS__FmainHead(CWrite_JS *THIS, CSContext *Actx) {
  MError__Freport("Can't use MAIN() in ZWT file");
}
void CWrite_JS__FmainEnd(CWrite_JS *THIS, CNode *Anode, CSContext *Actx) {
}
void CWrite_JS__FcopyC(CWrite_JS *THIS, CNode *Anode, CSContext *Actx) {
}
void CWrite_JS__FcopyJS(CWrite_JS *THIS, CNode *Anode, CSContext *Actx) {
  COutput__Fwrite(Actx->Vout, Anode->Vn_string);
}
void CWrite_JS__FwriteAlloc(CWrite_JS *THIS, char *AtypeName, CSContext *Actx) {
  COutput__Fwrite(Actx->Vout, Zconcat(Zconcat("new ", AtypeName), "()"));
}
void CWrite_JS__FwriteListAlloc(CWrite_JS *THIS, CSContext *Actx) {
  COutput__Fwrite(Actx->Vout, "[]");
}
void CWrite_JS__FwriteNewThis(CWrite_JS *THIS, CSymbol *Asym, CSContext *Actx) {
  COutput__FwriteIndent(Actx->Vout, 1);
  COutput__Fwrite(Actx->Vout, Zconcat(Zconcat("var ", CWrite_JS__X__VnewThisName), " = "));
  CWrite_JS__FwriteAlloc(THIS, Asym->VclassName, Actx);
  COutput__Fwrite(Actx->Vout, ";\n");
}
void CWrite_JS__FwriteNewReturn(CWrite_JS *THIS, COutput *Aout) {
  COutput__FwriteIndent(Aout, 1);
  COutput__Fwrite(Aout, Zconcat(Zconcat("return ", CWrite_JS__X__VnewThisName), ";\n"));
}
void CWrite_JS__FwriteSymName(CWrite_JS *THIS, CSymbol *Asym, CSContext *Actx) {
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
    COutput__Fwrite(Actx->Vout, Zconcat(Zconcat(Actx->Vscope->VthisName, "."), Asym->VcName));
  }
else
  {
    COutput__Fwrite(Actx->Vout, Asym->VcName);
  }
}
CSymbol *CWrite_JS__Fid(CWrite_JS *THIS, CNode *Anode, CSContext *Actx, CSymbol *AdestSym) {
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
    CWrite_JS__FwriteSymName(THIS, Vsym, Actx);
  }
  return Anode->Vn_returnSymbol;
}
CSymbol *CWrite_JS__FmethodReturnType(CWrite_JS *THIS, CNode *Anode, CSymbol *Asym, CSContext *Actx) {
  CSymbol *VretSym;
  VretSym = NULL;
  if ((Anode->Vn_type == 33))
  {
    VretSym = Actx->Vscope->Vclass;
  }
else if ((Anode->Vn_type == 32))
  {
    VretSym = CSymbol__FNEW(11);
  }
else if ((Anode->Vn_type == 34))
  {
    VretSym = MGenerate__FgenerateDeclType(Anode->Vn_returnType, CSContext__FcopyNoOut(Actx));
  }
  COutput__Fwrite(Actx->Vout, "function ");
  COutput__Fwrite(Actx->Vout, Zconcat(Asym->VcName, "("));
  return VretSym;
}
void CWrite_JS__FwriteMethodCall(CWrite_JS *THIS, CSymbol *Afunc, Zbool AmoreArgs, CSContext *Actx) {
  if ((Afunc->VclassName != NULL))
  {
    COutput__Fwrite(Actx->Vout, Zconcat(Zconcat(Zconcat(Actx->Vscope->VthisName, "."), Afunc->Vname), "("));
  }
else
  {
    COutput__Fwrite(Actx->Vout, Zconcat(Afunc->VcName, "("));
  }
}
void CWrite_JS__FargWithType(CWrite_JS *THIS, Zbool Afirst, CSymbol *AtypeSym, CNode *AtypeNode, char *AargName, CSContext *Actx) {
  if (!(Afirst))
  {
    COutput__Fwrite(Actx->Vout, ", ");
  }
  COutput__Fwrite(Actx->Vout, AargName);
}
Zbool CWrite_JS__FdoWriteDecl(CWrite_JS *THIS) {
  return 0;
}
CSymbol *CWrite_JS__Fsubscript(CWrite_JS *THIS, CNode *Anode, CSContext *Actx, CSymbol *AdestSym) {
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
void CWrite_JS__FiobjectMember(CWrite_JS *THIS, CSymbol *AobjSym, CSymbol *AitfSym, CNode *Anode, CSContext *Actx, CSymbol *AdestSym) {
  MGenerate__FgenExpr(Anode->Vn_left, Actx, AdestSym);
  COutput__Fwrite(Actx->Vout, Zconcat(".", AobjSym->VcName));
  Anode->Vn_undefined = Anode->Vn_left->Vn_undefined;
}
void CWrite_JS__FnumberOp(CWrite_JS *THIS, CNode *Anode, CSContext *Actx) {
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
void CWrite_JS__FconcatOp(CWrite_JS *THIS, CNode *Anode, CSContext *Actx) {
  MGenerate__FgenExpr(Anode->Vn_left, Actx, CSymbol__X__Vstring);
  COutput__Fwrite(Actx->Vout, " + ");
  MGenerate__FgenExpr(Anode->Vn_right, Actx, CSymbol__X__Vstring);
}
Zenum CWrite_JS__FplusOp(CWrite_JS *THIS, CNode *Anode, CSContext *Actx, Zenum AdestType) {
  CWrite_JS__FnumberOp(THIS, Anode, Actx);
  return 7;
}
void CWrite_JS__FincrdecrOp(CWrite_JS *THIS, CNode *Anode, CSContext *Actx) {
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
void CWrite_JS__FbooleanOp(CWrite_JS *THIS, CNode *Anode, CSContext *Actx) {
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
    MGenerate__FgenExpr(Anode->Vn_left, Actx, CSymbol__X__Vstring);
    if ((Anode->Vn_type == 91))
    {
      COutput__Fwrite(Actx->Vout, " == ");
    }
  else
    {
      COutput__Fwrite(Actx->Vout, " != ");
    }
    MGenerate__FgenExpr(Anode->Vn_right, Actx, CSymbol__X__Vstring);
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
void CWrite_JS__FcompareOp(CWrite_JS *THIS, CNode *Anode, CSContext *Actx) {
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
void CWrite_JS__FandorOp(CWrite_JS *THIS, CNode *Anode, CSContext *Actx) {
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
CSymbol *CWrite_JS__Fparens(CWrite_JS *THIS, CNode *Anode, CSContext *Actx, CSymbol *AdestSym) {
  COutput__Fwrite(Actx->Vout, "(");
  CSymbol *Vret;
  Vret = MGenerate__FgenExpr(Anode->Vn_left, Actx, AdestSym);
  COutput__Fwrite(Actx->Vout, ")");
  return Vret;
}
CSymbol *CWrite_JS__FaltOp(CWrite_JS *THIS, CNode *Anode, CSContext *Actx, CSymbol *AdestSym) {
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
CSymbol *CWrite_JS__FlistPart(CWrite_JS *THIS, CNode *Anode, CSContext *Actx, CSymbol *AdestSym) {
  return MListStuff__FgenerateListPart_JS(Anode, Actx, AdestSym);
}
void CWrite_JS__FdictPart(CWrite_JS *THIS, CNode *Anode, CSymbol *Aret, CSContext *Actx) {
  MDictStuff__FgenerateDictPart_C(Anode, Aret, Actx);
}
CSymbol *CWrite_JS__Fexpr(CWrite_JS *THIS, CNode *Anode, CSContext *Actx, CSymbol *AdestSym) {
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
          COutput__Fwrite(Actx->Vout, "\"\" +");
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
char *CWrite_JS__FwriteImport(CWrite_JS *THIS, CZimbuFile *Aimport, COutput__X__CGroup *AmyOuts, CScope *Ascope) {
  char *VmoduleName;
  VmoduleName = CZimbuFile__FgetModuleName(Aimport);
  
#if defined(__MINGW32__) || defined(_MSC_VER)
_mkdir(Aimport->VoutDir)
#else
mkdir(Aimport->VoutDir, 0777)
#endif
;
  char *Vfname;
  Vfname = Zconcat(Zconcat(Zconcat(Zconcat(Zconcat(Aimport->VoutDir, "/"), VmoduleName), "."), THIS->VpermuName), ".html");
  (fputs(Zconcat(Zconcat("Writing file ", Vfname), "..."), stdout) | fputc('\n', stdout));
  FILE *Vfd;
  Vfd = fopen(Vfname, "w");
  if ((Vfd == NULL))
  {
    (fputs(Zconcat("ERROR: Cannot open file for writing: ", Vfname), stdout) | fputc('\n', stdout));
    exit(1);
  }
  fputs("<html>\n  <head>\n    <script>\n      var $wnd = parent;\n      var $doc = $wnd.document;\n    </script>\n  </head>\n  <body>\n    <script><!--\n", Vfd);
  if (!(COutput__X__CFragmentHead__Fempty(AmyOuts->VtypeOut->Vhead1)))
  {
    fputs("// typedefs\n", Vfd);
    COutput__X__CFragmentHead__Fwrite__1(AmyOuts->VtypeOut->Vhead1, Vfd);
  }
  fputs("\n// structs\n", Vfd);
  COutput__X__CFragmentHead__Fwrite__1(AmyOuts->VstructOut->Vhead1, Vfd);
  fputs("\n// declarations\n", Vfd);
  CWrite_JS__X__FwriteDecl(Aimport->VtopScope, Vfd);
  COutput__X__CFragmentHead__Fwrite__1(AmyOuts->VdeclOut->Vhead1, Vfd);
  fputs("\n// bodies\n", Vfd);
  CWrite_JS__X__FwriteBodies(Aimport->VtopScope, Vfd);
  COutput__X__CFragmentHead__Fwrite__1(AmyOuts->VbodyOut->Vhead1, Vfd);
  fputs(Zconcat(Zconcat("\nM", VmoduleName), "__Finit();\n"), Vfd);
  fputs("    --></script>\n  </body>\n</html>\n", Vfd);
  fclose(Vfd);
  (fputs("Done.", stdout) | fputc('\n', stdout));
  return Vfname;
}
void CWrite_JS__FwriteIncludeImport(CWrite_JS *THIS, CZimbuFile *Aimport, COutput__X__CGroup *AmyOuts, CScope *Ascope) {
  char *Vpre;
  Vpre = Zconcat("// including ", Aimport->VrootName);
  if (!(COutput__X__CFragmentHead__Fempty(AmyOuts->VtypeOut->Vhead1)))
  {
    COutput__Fwrite(AmyOuts->VtypeOut, Zconcat(Vpre, " typedefs\n"));
    COutput__X__CFragmentHead__Fappend__1(AmyOuts->VtypeOut->Vhead1, Aimport->Vjs->Vheads->Vtypedefs);
  }
  COutput__Fwrite(AmyOuts->VstructOut, Zconcat(Vpre, " structs\n"));
  COutput__X__CFragmentHead__Fappend__1(AmyOuts->VstructOut->Vhead1, Aimport->Vjs->Vheads->Vstructs);
  COutput__Fwrite(AmyOuts->VdeclOut, Zconcat(Vpre, " declarations\n"));
  COutput__X__CFragmentHead__Fappend__1(AmyOuts->VdeclOut->Vhead1, Aimport->Vjs->Vheads->Vdeclares);
  COutput__Fwrite(AmyOuts->VbodyOut, Zconcat(Vpre, " bodies\n"));
  COutput__X__CFragmentHead__Fappend__1(AmyOuts->VbodyOut->Vhead1, Aimport->Vjs->Vheads->VfuncBodies);
}
Zbool CWrite_JS__FneedWrite(CWrite_JS *THIS, CZimbuFile *AzimbuFile) {
  if (((AzimbuFile->Vjs->VstartedWrite != NULL) && (Zstrcmp(AzimbuFile->Vjs->VstartedWrite, THIS->VpermuName) == 0)))
  {
    return 0;
  }
  AzimbuFile->Vjs->VstartedWrite = THIS->VpermuName;
  CZimbuFile__X__CCodeSpecific__Fclear(AzimbuFile->Vjs);
  return 1;
}
void CWrite_JS__FwriteClassDef(CWrite_JS *THIS, char *Aname, COutput *AtypeOut) {
}
void CWrite_JS__FwriteClassDecl(CWrite_JS *THIS, CSymbol *AclassSym, COutput__X__CGroup *Aouts, COutput *AstructOut) {
  char *Vname;
  Vname = AclassSym->VcName;
  COutput__Fwrite(Aouts->VstructOut, Zconcat(Zconcat("function ", Vname), "() {\n"));
  COutput__Fwrite(Aouts->VstructOut, "}\n");
  COutput__Fwrite(Aouts->VstructOut, Zconcat(Zconcat("_ = ", Vname), ".prototype = "));
  if ((AclassSym->VparentClass != NULL))
  {
    COutput__Fwrite(Aouts->VstructOut, Zconcat(Zconcat("new ", AclassSym->VparentClass->VcName), "();\n"));
  }
else
  {
    COutput__Fwrite(Aouts->VstructOut, "{};\n");
  }
  if ((AclassSym->VmemberList != NULL))
  {
    {
      Zfor_T *Zf = ZforNew(AclassSym->VmemberList, 2);
      CSymbol *Vs;
      for (ZforGetPtr(Zf, (char **)&Vs); ZforCont(Zf); ZforNextPtr(Zf, (char **)&Vs)) {
        if (CNode__X__FisMethodType(Vs->Vtype))
        {
          if ((Zstrcmp(Vs->Vname, "NEW") != 0))
          {
            COutput__Fwrite(Aouts->VstructOut, Zconcat(Zconcat("_.", Vs->Vname), " = "));
            COutput__Fwrite(Aouts->VstructOut, Vs->VcName);
            COutput__Fwrite(Aouts->VstructOut, ";\n");
            if ((((Vs->Vattributes) & 16)))
            {
              CSymbol *Vparent;
              Vparent = AclassSym->VparentClass;
              while ((Vparent != NULL))
              {
                CSymbol *VotherSym;
                VotherSym = CSymbol__FfindMatchingFunction(Vparent, Vs->Vname, Vs->VmemberList, NULL, 0, 0);
                if ((VotherSym != NULL))
                {
                  COutput__Fwrite(Aouts->VstructOut, Zconcat(Zconcat("_.parent__", Vs->Vname), " = "));
                  COutput__Fwrite(Aouts->VstructOut, VotherSym->VcName);
                  COutput__Fwrite(Aouts->VstructOut, ";\n");
                  break;
                }
                Vparent = Vparent->VparentClass;
              }
            }
          }
        }
      else
        {
          COutput__Fwrite(Aouts->VstructOut, Zconcat(Zconcat("_.", Vs->VcName), " = "));
          CWrite_JS__FdefaultInit(THIS, Vs, Aouts->VstructOut);
          COutput__Fwrite(Aouts->VstructOut, ";\n");
        }
      }
    }
  }
}
void CWrite_JS__FdefaultInit(CWrite_JS *THIS, CSymbol *Asym, COutput *Aout) {
  if (CNode__X__FisPointerType(Asym->Vtype))
  {
    COutput__Fwrite(Aout, "null");
  }
else
  {
    COutput__Fwrite(Aout, "0");
  }
}
void CWrite_JS__Fnil(CWrite_JS *THIS, CSContext *Actx) {
  COutput__Fwrite(Actx->Vout, "null");
}
void CWrite_JS__Fmember(CWrite_JS *THIS, char *Aname, COutput *Aout) {
  COutput__Fwrite(Aout, Zconcat(".", Aname));
}
void CWrite_JS__Fvardecl(CWrite_JS *THIS, CSContext *Actx, COutput *Aout) {
  if ((CScope__FisClassScope(Actx->Vscope) && !(Actx->Vscope->Vstatements)))
  {
    COutput__Fwrite(Aout, Zconcat(Actx->Vscope->VthisName, "."));
  }
else
  {
    COutput__Fwrite(Aout, "var ");
  }
}
void CWrite_JS__Fvartype(CWrite_JS *THIS, CSymbol *Asym, CNode *Anode, CScope *Ascope, COutput *Aout) {
}
void CWrite_JS__FforStart(CWrite_JS *THIS, COutput *Aout) {
  COutput__Fwrite(Aout, "var Zf = ZforNew(");
}
void CWrite_JS__FforLoop(CWrite_JS *THIS, CSymbol *AvarSym, COutput *Aout) {
  COutput__Fwrite(Aout, "for (");
  COutput__Fwrite(Aout, Zconcat(Zconcat(Zconcat(AvarSym->VcName, " = ZforGet(Zf, "), AvarSym->VcName), "); "));
  COutput__Fwrite(Aout, "ZforCont(Zf); ");
  COutput__Fwrite(Aout, Zconcat(Zconcat(Zconcat(AvarSym->VcName, " = ZforNext(Zf, &"), AvarSym->VcName), ")"));
  COutput__Fwrite(Aout, ") {\n");
}
void CWrite_JS__X__FwriteDecl(CScope *AtopScope, FILE *Afd) {
  if (ZDictHas(AtopScope->VusedItems, 0, "for"))
  {
    fputs("// for used\n", Afd);
  }
}
void CWrite_JS__X__FwriteBodies(CScope *AtopScope, FILE *Afd) {
  if ((ZDictHas(AtopScope->VusedItems, 0, "list") || MDictStuff__VuseDict))
  {
    fputs("\nfunction Zerror(msg) {\n  alert(\"ERROR: \" + msg);\n}\n", Afd);
  }
  MListStuff__FwriteBody_JS(AtopScope, Afd);
}
void Iwrite_js() {
 static int done = 0;
 if (!done) {
  done = 1;
  Idictstuff();
  Igenerate();
  Iliststuff();
  Ioutput();
  Iresolve();
  Iscontext();
  Iscope();
  Isymbol();
  Izimbufile();
  CWrite_JS__X__VnewThisName = "thisO";
 }
}
