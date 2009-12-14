#define INC_generate_B 1
/*
 * FUNCTION BODIES
 */
#ifndef INC_builtin_B
#include "../ZUDIR/builtin.b.c"
#endif
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
#ifndef INC_expreval_B
#include "../ZUDIR/expreval.b.c"
#endif
#ifndef INC_input_B
#include "../ZUDIR/input.b.c"
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
#ifndef INC_parse_B
#include "../ZUDIR/parse.b.c"
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
#ifndef INC_usedfile_B
#include "../ZUDIR/usedfile.b.c"
#endif
#ifndef INC_write_c_B
#include "../ZUDIR/write_c.b.c"
#endif
#ifndef INC_write_js_B
#include "../ZUDIR/write_js.b.c"
#endif
#ifndef INC_zimbufile_B
#include "../ZUDIR/zimbufile.b.c"
#endif
#ifndef INC_zwtvalues_B
#include "../lib/ZUDIR/zwtvalues.b.c"
#endif
Zbool MGenerate__FdoError(COutput *Aout) {
  return (MGenerate__VshowErrors && Aout->Vwriting);
}
Zbool MGenerate__Fresolve(CUsedFile *AusedFile) {
  ++(AusedFile->VzimbuFile->VstartedPass);
  return MGenerate__Fresolve__1(AusedFile, "");
}
Zbool MGenerate__Fresolve__1(CUsedFile *AusedFile, char *Aindent) {
  char *Vleader;
  Vleader = Zconcat(Aindent, AusedFile->VzimbuFile->Vfilename);
  CScope *VtopScope;
  VtopScope = CUsedFile__Fscope(AusedFile);
  if ((VtopScope->VtopNode == NULL))
  {
    MError__FverboseMsg(Zconcat(Vleader, ": Empty file\n"));
    return 0;
  }
  Zint VpreviousUndef;
  VpreviousUndef = VtopScope->VtopNode->Vn_undefined;
  COutput__X__CGroup *Vdummy;
  Vdummy = COutput__X__CGroup__FNEW__1();
  COutput__X__CGroup__FsetHeads(Vdummy, COutput__X__CHeads__FNEW__1());
  Zoref *Vgen;
  Vgen = ZallocZoref(CResolve__FNEW(), 0);
  MError__FverboseMsg(Zconcat(Vleader, ": Check imports...\n"));
  Zint Vundef;
  Vundef = MGenerate__FgenerateImports(AusedFile, Vgen, Vdummy);
  if ((VtopScope->Vpass > AusedFile->VzimbuFile->VstartedPass))
  {
    return (VtopScope->VtopNode->Vn_undefined > 0);
  }
  MError__FverboseMsg(Zconcat(Zconcat(Zconcat(Vleader, ": Pass "), Zint2string(VtopScope->Vpass)), "...\n"));
  Vundef += MGenerate__Fgenerate(VtopScope->VtopNode, VtopScope, Vgen, Vdummy);
  if ((VtopScope->VtopNode != NULL))
  {
    VtopScope->VtopNode->Vn_undefined = Vundef;
  }
  MError__FverboseMsg(Zconcat(Zconcat(Zconcat(Vleader, ": Pass "), Zint2string(VtopScope->Vpass)), " done."));
  if ((VtopScope->VtopNode->Vn_undefined > 0))
  {
    MError__FverboseMsg(Zconcat(Zconcat(" (", Zint2string(VtopScope->VtopNode->Vn_undefined)), " undefined symbols)"));
  }
  MError__FverboseMsg("\n");
  ++(VtopScope->Vpass);
  return ((VtopScope->VtopNode->Vn_undefined > 0) && (((VtopScope->Vpass == 2) || (VtopScope->VtopNode->Vn_undefined < VpreviousUndef))));
}
void MGenerate__Fwrite(CUsedFile *AusedFile, COutput__X__CGroup *Aoutputs) {
  MGenerate__Fwrite__1(AusedFile, ZallocZoref(CWrite_C__FNEW(), 1), Aoutputs);
}
void MGenerate__Fwrite__1(CUsedFile *AusedFile, Zoref *Agen, COutput__X__CGroup *Aoutputs) {
  char *VinFileName;
  VinFileName = AusedFile->VzimbuFile->Vfilename;
  CScope *VtopScope;
  VtopScope = CUsedFile__Fscope(AusedFile);
  char *Vleader;
  Vleader = Zconcat(VtopScope->VimportIndent, VinFileName);
  char *Vname;
  Vname = CResolve_I__MgetLangName_ptr[Agen->type](Agen->ptr);
  if (((*(char **)(Agen->ptr + CResolve_I__VpermuName_off[Agen->type])) != NULL))
  {
    Vname = Zconcat(Zconcat((*(char **)(Agen->ptr + CResolve_I__VpermuName_off[Agen->type])), " "), Vname);
  }
  MError__FverboseMsg(Zconcat(Zconcat(Zconcat(Vleader, ": Generating "), Vname), " code...\n"));
  MGenerate__VshowErrors = 1;
  COutput__X__CGroup__FstartWriting(Aoutputs);
  MGenerate__FgenerateImports(AusedFile, Agen, Aoutputs);
  MGenerate__Fgenerate(VtopScope->VtopNode, VtopScope, Agen, Aoutputs);
  MError__FverboseMsg(Zconcat(Vleader, ": Done.\n"));
}
Zint MGenerate__FgenerateImports(CUsedFile *AusedFile, Zoref *Agen, COutput__X__CGroup *Aouts) {
  CScope *Vscope;
  Vscope = CUsedFile__Fscope(AusedFile);
  Zint Vundef;
  Vundef = CBuiltin__X__FgenerateBuiltins(AusedFile, Agen, Aouts);
  CNode *Vnode;
  Vnode = Vscope->VtopNode;
  while (((Vnode != NULL) && (Vnode->Vn_type == 38)))
  {
    Vundef += MGenerate__FgenerateImport(AusedFile, Vnode, Agen, Aouts);
    Vnode = Vnode->Vn_next;
  }
  return Vundef;
}
Zint MGenerate__Fgenerate(CNode *AstartNode, CScope *Ascope, Zoref *Agen, COutput__X__CGroup *Aouts) {
  Zint Vundef;
  Vundef = MGenerate__FgenerateNode(AstartNode, Ascope, Agen, Aouts);
  if ((AstartNode != NULL))
  {
    AstartNode->Vn_undefined = Vundef;
    if (((Aouts->Vout->Vwriting && (Ascope->VmemberList != NULL)) && ((*(Zenum *)(Agen->ptr + CResolve_I__VtargetLang_off[Agen->type])) == 1)))
    {
      {
        Zfor_T *Zf = ZforNew(Ascope->VmemberList, 2);
        CSymbol *Vsym;
        for (ZforGetPtr(Zf, (char **)&Vsym); ZforCont(Zf); ZforNextPtr(Zf, (char **)&Vsym)) {
          if ((!(Vsym->VnoGenerate) && (((Vsym->Vtype == 21) || (Vsym->Vtype == 23)))))
          {
            MGenerate__FgenerateClassOffTable(Vsym, CSContext__FNEW(Ascope, Agen, Aouts->VdeclOut));
          }
        }
      }
      COutput__Fappend(Aouts->VdeclOut, MGenerate__VvirtualOut);
      COutput__Fclear(MGenerate__VvirtualOut);
    }
  }
  return Vundef;
}
Zint MGenerate__FgenerateNode(CNode *AstartNode, CScope *Ascope, Zoref *Agen, COutput__X__CGroup *Aouts) {
  CSContext *Vctx;
  Vctx = CSContext__FNEW(Ascope, Agen, Aouts->Vout);
  Zint Vundef;
  Vundef = 0;
  CNode *Vnode;
  Vnode = AstartNode;
  while ((Vnode != NULL))
  {
    if ((Vnode->Vn_type != 38))
    {
      Vundef += MGenerate__FgenerateOneNode(&(Vnode), Vctx, Aouts);
    }
    Vnode = Vnode->Vn_next;
  }
  return Vundef;
}
Zint MGenerate__FgenerateOneNode(CNode * *Rref, CSContext *Actx, COutput__X__CGroup *Aouts) {
  COutput *Vout;
  Vout = Actx->Vout;
  CNode *Vnode;
  Vnode = (*Rref);
  Zint Vundef;
  Vundef = 0;
  Zoref *Vgen;
  Vgen = Actx->Vgen;
  CScope *Vscope;
  Vscope = Actx->Vscope;
  if (MError__Vdebug)
  {
    if ((Vscope->VimportIndent != NULL))
    {
      fputs(Vscope->VimportIndent, stdout);
    }
    fputs("generateOneNode() ", stdout);
    fputs(Zconcat(Zenum2string(CNode__X__EType, sizeof(CNode__X__EType) / sizeof(char *), Vnode->Vn_type), " node"), stdout);
    if ((Vnode->Vn_string != NULL))
    {
      fputs(Zconcat(Zconcat(" \"", Vnode->Vn_string), "\""), stdout);
    }
    fputs("\n", stdout);
  }
  switch (Vnode->Vn_type)
  {
  case 40:
    {
      {
        COutput__Fwrite(Actx->Vout, Vnode->Vn_string);
        return 0;
      }
        break;
    }
  case 41:
    {
      {
        COutput__FwriteIndent(Vout, Vscope->Vdepth);
        COutput__Fwrite(Vout, Vnode->Vn_string);
        COutput__Fwrite(Vout, ";\n");
        return 0;
      }
        break;
    }
  case 38:
    {
      {
        MError__Freport("INTERNAL: unexpected IMPORT");
        return 0;
      }
        break;
    }
  case 39:
    {
      {
        return MGenerate__FgenerateMain(Vnode, Actx, Aouts);
      }
        break;
    }
  case 19:
    {
      {
        return MGenerate__FgenerateModule(Vnode, Actx, Aouts);
      }
        break;
    }
  case 21:
  case 23:
    {
      {
        return MGenerate__FgenerateClass(Vnode, Actx, Aouts);
      }
        break;
    }
  case 22:
    {
      {
        return MGenerate__FgenerateShared(Vnode, Actx, Aouts);
      }
        break;
    }
  case 28:
    {
      {
        return MGenerate__FgenerateBits(Vnode, Actx, Aouts);
      }
        break;
    }
  case 31:
    {
      {
        return MGenerate__FgenerateEnum(Vnode, Actx, Aouts);
      }
        break;
    }
  case 32:
  case 33:
  case 35:
  case 34:
    {
      {
        return MGenerate__FgenerateMethod(Vnode, Actx, Aouts);
      }
        break;
    }
  case 66:
    {
      {
        return MGenerate__FgenerateDeclare(Vnode, Actx, Aouts);
      }
        break;
    }
  case 42:
    {
      {
        return MGenerate__FgenerateGenerateIf(Vnode, Actx, Aouts);
      }
        break;
    }
  }
  if (!(Vscope->Vstatements))
  {
    CNode__Ferror(Vnode, "Item not allowed at top level");
    return Vundef;
  }
  switch (Vnode->Vn_type)
  {
  case 58:
    {
      {
        return MGenerate__FgenerateScope(Vnode->Vn_left, Actx, Vnode->Vn_nodeType, NULL, Aouts);
      }
        break;
    }
  case 45:
    {
      {
        COutput__FwriteIndent(Vout, Vscope->Vdepth);
        COutput__Fwrite(Vout, "if (");
        MGenerate__FgenExpr(Vnode->Vn_cond, Actx, CSymbol__X__Vbool);
        COutput__Fwrite(Vout, ")\n");
        Vundef = Vnode->Vn_cond->Vn_undefined;
      }
        break;
    }
  case 47:
    {
      {
        COutput__FwriteIndent(Vout, (Vscope->Vdepth - 1));
        COutput__Fwrite(Vout, "else if (");
        MGenerate__FgenExpr(Vnode->Vn_cond, Actx, CSymbol__X__Vbool);
        COutput__Fwrite(Vout, ")\n");
        Vundef = Vnode->Vn_cond->Vn_undefined;
      }
        break;
    }
  case 46:
    {
      {
        COutput__FwriteIndent(Vout, (Vscope->Vdepth - 1));
        COutput__Fwrite(Vout, "else\n");
      }
        break;
    }
  case 48:
    {
      {
        COutput__FwriteIndent(Vout, Vscope->Vdepth);
        COutput__Fwrite(Vout, "while (");
        MGenerate__FgenExpr(Vnode->Vn_cond, Actx, CSymbol__X__Vbool);
        COutput__Fwrite(Vout, ")\n");
        Vundef = Vnode->Vn_cond->Vn_undefined;
        Vundef += MGenerate__FgenerateScope(Vnode->Vn_right, Actx, 48, NULL, Aouts);
      }
        break;
    }
  case 67:
    {
      {
        COutput__FwriteIndent(Vout, Vscope->Vdepth);
        COutput__Fwrite(Vout, "do\n");
      }
        break;
    }
  case 68:
    {
      {
        COutput__FwriteIndent(Vout, Vscope->Vdepth);
        COutput__Fwrite(Vout, "  while (!(");
        MGenerate__FgenExpr(Vnode->Vn_cond, Actx, CSymbol__X__Vbool);
        COutput__Fwrite(Vout, "));\n");
        Vundef = Vnode->Vn_cond->Vn_undefined;
      }
        break;
    }
  case 49:
    {
      {
        Vundef = MGenerate__FgenerateFor(Vnode, Actx, Aouts);
      }
        break;
    }
  case 71:
  case 72:
    {
      {
        Vundef = MGenerate__FgenerateReturnExit(Vnode, Actx, Aouts);
      }
        break;
    }
  case 51:
    {
      {
        MGenerate__FcheckScope(Vnode, Vscope, 1);
        COutput__FwriteIndent(Vout, Vscope->Vdepth);
        COutput__Fwrite(Vout, "break;\n");
      }
        break;
    }
  case 52:
    {
      {
        MGenerate__FcheckScope(Vnode, Vscope, 0);
        COutput__FwriteIndent(Vout, Vscope->Vdepth);
        COutput__Fwrite(Vout, "continue;\n");
      }
        break;
    }
  case 53:
    {
      {
        CNode__Ferror(Vnode, "PROCEED not allowed here");
      }
        break;
    }
  case 63:
    {
      {
        Vundef = MGenerate__FgenerateSwitch(Vnode, Actx, Aouts);
      }
        break;
    }
  case 64:
  case 65:
    {
      {
        Vundef = MGenerate__FgenerateCase(Rref, Actx, Aouts);
      }
        break;
    }
  case 62:
    {
      {
        COutput__FwriteIndent(Vout, Vscope->Vdepth);
        MGenerate__FgenerateCall(Vnode->Vn_left, Actx, NULL, 0);
        Vundef = Vnode->Vn_left->Vn_undefined;
        COutput__Fwrite(Vout, ";\n");
      }
        break;
    }
  case 69:
  case 70:
    {
      {
        Vundef = MGenerate__FgenerateIncDec(Vnode, Actx, Aouts);
      }
        break;
    }
  case 54:
  case 55:
  case 56:
  case 57:
    {
      {
        Vundef = MGenerate__FgenerateAssign(Vnode, Actx, Aouts);
      }
        break;
    }
  default:
    {
      {
        MGenerate__Ferror(Zconcat(Zconcat("INTERNAL: generate(): Node type \"", Zenum2string(CNode__X__EType, sizeof(CNode__X__EType) / sizeof(char *), Vnode->Vn_type)), "\" not supported"), Vnode);
      }
        break;
    }
  }
  return Vundef;
}
Zint MGenerate__FgenerateMain(CNode *Anode, CSContext *Actx, COutput__X__CGroup *Aouts) {
  Zoref *Vgen;
  Vgen = Actx->Vgen;
  CScope *Vscope;
  Vscope = Actx->Vscope;
  if (((Vscope->Vouter != NULL) || (Vscope->VscopeName != NULL)))
  {
    MGenerate__Ferror("MAIN not at toplevel", Anode);
  }
  if ((((MGenerate__Vskip_zero_undefined && !(MGenerate__FdoError(Actx->Vout))) && (Vscope->Vpass > 1)) && (Anode->Vn_undefined == 0)))
  {
    return 0;
  }
  CResolve_I__MmainHead__CSContext_ptr[Vgen->type](Vgen->ptr, Actx);
  CScope *VmainScope;
  VmainScope = CScope__X__FnewScope(Vscope, 0);
  VmainScope->VreturnSymbol = CSymbol__FNEW(7);
  VmainScope->VscopeName = "FMAIN";
  VmainScope->VnodeType = 35;
  COutput__X__CGroup *VnewOuts;
  VnewOuts = COutput__X__CGroup__Fcopy__1(Aouts);
  VnewOuts->VvarOut = COutput__Fcopy(VnewOuts->Vout);
  VmainScope->VtopNode = Anode->Vn_left;
  Anode->Vn_undefined = MGenerate__Fgenerate(VmainScope->VtopNode, VmainScope, Vgen, VnewOuts);
  CResolve_I__MmainEnd__CNode__CSContext_ptr[Vgen->type](Vgen->ptr, Anode, Actx);
  return Anode->Vn_undefined;
}
Zint MGenerate__FgenerateModule(CNode *Anode, CSContext *Actx, COutput__X__CGroup *Aouts) {
  COutput *Vout;
  Vout = Actx->Vout;
  CScope *Vscope;
  Vscope = Actx->Vscope;
  Zint Vundef;
  Vundef = 0;
  if (((Vscope->VscopeName != NULL) && (Vscope->VscopeName[0] != 77)))
  {
    CNode__Ferror(Anode, "MODULE can only be defined inside a MODULE");
  }
  CNode__FcheckTypeName(Anode, "module");
  CSymbol *VscopeSym;
  VscopeSym = NULL;
  if ((Vscope->Vpass <= 1))
  {
    VscopeSym = CScope__FaddSymbol(Vscope, Anode->Vn_string, 19, Anode, 0);
    if ((Vscope->Vpass == 1))
    {
      Anode->Vn_symbol = VscopeSym;
    }
    if ((Vscope->VscopeName == NULL))
    {
      Anode->Vn_scopeName = Zconcat("M", Anode->Vn_string);
    }
  else
    {
      Anode->Vn_scopeName = Zconcat(Zconcat(Vscope->VscopeName, "__M"), Anode->Vn_string);
    }
    VscopeSym->VcName = Anode->Vn_scopeName;
    VscopeSym->Vscope = Vscope;
  }
else
  {
    if ((Anode->Vn_symbol == NULL))
    {
      CNode__Ferror(Anode, "INTERNAL: module n_symbol is NIL");
      return 0;
    }
    VscopeSym = Anode->Vn_symbol;
    if (!(Vscope->VforwardDeclare))
    {
      CScope__FaddMember(Vscope, VscopeSym);
    }
    if (((MGenerate__Vskip_zero_undefined && !(MGenerate__FdoError(Vout))) && (Anode->Vn_undefined == 0)))
    {
      return 0;
    }
  }
  COutput__X__CGroup *VnewOuts;
  VnewOuts = Aouts;
  if ((((((VscopeSym->Vattributes) & 32)) && Vout->Vwriting) && ((*(Zenum *)(Actx->Vgen->ptr + CResolve_I__VtargetLang_off[Actx->Vgen->type])) == 1)))
  {
    char *Vfname;
    Vfname = Zconcat(VscopeSym->VcName, "__FINFO()");
    COutput__Fwrite(Aouts->VbodyOut, Zconcat(Zconcat("MINFOmodule__CModuleInfo *", Vfname), " {\n"));
    COutput__Fwrite(Aouts->VbodyOut, Zconcat(Zconcat("  MINFOmodule__CModuleInfo *info = ", "(MINFOmodule__CModuleInfo *)Zalloc("), "sizeof(MINFOmodule__CModuleInfo));\n"));
    COutput__Fwrite(Aouts->VbodyOut, Zconcat(Zconcat("  info->Vname = \"", Anode->Vn_string), "\";\n"));
    COutput__Fwrite(Aouts->VbodyOut, "  info->Vpermutations = ZnewDict(1);\n");
    if ((VscopeSym->VzwtPermu != NULL))
    {
      {
        Zfor_T *Zf = ZforNew(ZDictKeys(VscopeSym->VzwtPermu), 2);
        char *Vkey;
        for (ZforGetPtr(Zf, (char **)&Vkey); ZforCont(Zf); ZforNextPtr(Zf, (char **)&Vkey)) {
          COutput__Fwrite(Aouts->VbodyOut, Zconcat(Zconcat(Zconcat(Zconcat("  ZDictAdd(0, info->Vpermutations, 0, \"", Vkey), "\", 0, \""), ZDictGetPtr(VscopeSym->VzwtPermu, 0, Vkey)), "\");\n"));
        }
      }
    }
    COutput__Fwrite(Aouts->VbodyOut, "  return info;\n");
    COutput__Fwrite(Aouts->VbodyOut, "}\n");
    COutput__Fwrite(Aouts->VdeclOut, Zconcat(Zconcat("MINFOmodule__CModuleInfo *", Vfname), ";\n"));
  }
  if (((((*(Zenum *)(Actx->Vgen->ptr + CResolve_I__VtargetLang_off[Actx->Vgen->type])) == 0) || ((((*(Zenum *)(Actx->Vgen->ptr + CResolve_I__VtargetLang_off[Actx->Vgen->type])) == 1) && CScope__FusedAsZimbu(Vscope)))) || ((((*(Zenum *)(Actx->Vgen->ptr + CResolve_I__VtargetLang_off[Actx->Vgen->type])) == 2) && CScope__FusedAsZwt(Vscope)))))
  {
    CScope *VmoduleScope;
    VmoduleScope = CScope__X__FnewScope(Vscope, 1);
    VmoduleScope->VscopeName = Anode->Vn_scopeName;
    VmoduleScope->VmoduleName = Anode->Vn_string;
    VmoduleScope->Vdepth = 0;
    VmoduleScope->VreturnSymbol = NULL;
    VmoduleScope->Vstatements = 0;
    VmoduleScope->VmemberList = VscopeSym->VmemberList;
    VmoduleScope->VtopNode = Anode->Vn_left;
    Vundef = MGenerate__Fgenerate(VmoduleScope->VtopNode, VmoduleScope, Actx->Vgen, Aouts);
    Anode->Vn_undefined = Vundef;
    VscopeSym->VmemberList = VmoduleScope->VmemberList;
  }
  return Vundef;
}
Zint MGenerate__FgenerateClass(CNode *Anode, CSContext *Actx, COutput__X__CGroup *Aouts) {
  COutput *Vout;
  Vout = Actx->Vout;
  Zint Vundef;
  Vundef = 0;
  Zoref *Vgen;
  Vgen = Actx->Vgen;
  CScope *Vscope;
  Vscope = Actx->Vscope;
  CNode__FcheckTypeName(Anode, ((Anode->Vn_type == 21)) ? ("class") : ("interface"));
  char *Vname;
  Vname = Anode->Vn_string;
  CSymbol *VclassSym;
  VclassSym = NULL;
  if ((Vscope->Vpass <= 1))
  {
    if ((((Zstrcmp(Vname, Vscope->VmoduleName) == 0) && (Vscope->VmemberList != NULL)) && (Vscope->VmemberList->itemCount >= 1)))
    {
      MGenerate__Ferror(Zconcat(Zconcat(Zconcat("CLASS ", Vname), " must be the first item in module "), Vname), Anode);
    }
    VclassSym = CScope__FaddSymbol(Vscope, Vname, Anode->Vn_type, Anode, (Zstrcmp(Vname, Vscope->VmoduleName) == 0));
    if ((Vscope->Vpass == 1))
    {
      Anode->Vn_symbol = VclassSym;
    }
    if ((Vscope->VscopeName == NULL))
    {
      Anode->Vn_scopeName = Zconcat("C", Vname);
    }
  else
    {
      Anode->Vn_scopeName = Zconcat(Zconcat(Vscope->VscopeName, "__C"), Vname);
    }
  }
else
  {
    VclassSym = Anode->Vn_symbol;
    if (!(Vscope->VforwardDeclare))
    {
      CScope__FaddMember(Vscope, VclassSym);
    }
    VclassSym->VnoGenerate = Vscope->VnoGenerate;
    if (((MGenerate__Vskip_zero_undefined && !(MGenerate__FdoError(Vout))) && (Anode->Vn_undefined == 0)))
    {
      return 0;
    }
  }
  VclassSym->VcName = Anode->Vn_scopeName;
  VclassSym->VclassName = Anode->Vn_scopeName;
  VclassSym->Vattributes = Anode->Vn_attr;
  if ((Anode->Vn_cond != NULL))
  {
    Anode->Vn_cond->Vn_undefined = 0;
    VclassSym->VparentClass = CScope__FfindNodeSymbol(Vscope, Anode->Vn_cond);
    Vundef += Anode->Vn_cond->Vn_undefined;
    if (MGenerate__FdoError(Vout))
    {
      if ((VclassSym->VparentClass == NULL))
      {
        CNode__Ferror(Anode, "Class not found");
      }
    else if ((VclassSym->VparentClass->Vtype != 21))
      {
        CNode__Ferror(Anode, "Not a class");
      }
    else if ((((VclassSym->VparentClass->Vattributes) & 4)))
      {
        CNode__Ferror(Anode, Zconcat(Zconcat("Cannot extend ", VclassSym->VparentClass->Vname), ": it is marked final"));
      }
    }
    if ((VclassSym->VparentClass != NULL))
    {
      CSymbol__FaddChild(VclassSym->VparentClass, VclassSym);
    }
  }
  if ((Anode->Vn_right != NULL))
  {
    VclassSym->Vinterfaces = NULL;
    Vundef += MGenerate__FhandleImplements(Anode->Vn_right, VclassSym, Vscope, MGenerate__FdoError(Vout));
  }
  COutput *VstructOut;
  VstructOut = COutput__FNEW(COutput__X__CFragmentHead__FNEW__1());
  VstructOut->Vwriting = Vout->Vwriting;
  if (((VclassSym->VparentClass != NULL) && (VclassSym->VparentClass->VstructOut != NULL)))
  {
    COutput__Fappend(VstructOut, VclassSym->VparentClass->VstructOut);
  }
  if ((!((((VclassSym->Vattributes) & 1))) && Vout->Vwriting))
  {
    CResolve_I__MwriteClassDef__string__COutput_ptr[Vgen->type](Vgen->ptr, Anode->Vn_scopeName, Aouts->VtypeOut);
  }
  CScope *VclassScope;
  VclassScope = CScope__X__FnewScope(Vscope, 1);
  VclassScope->VscopeName = Anode->Vn_scopeName;
  VclassScope->Vclass = VclassSym;
  VclassScope->VinsideShared = 0;
  VclassScope->Vdepth = 1;
  VclassScope->VreturnSymbol = NULL;
  VclassScope->Vinit = 0;
  VclassScope->Vstatements = 0;
  VclassScope->VmemberList = VclassSym->VmemberList;
  VclassScope->VtopNode = Anode->Vn_left;
  VclassScope->VthisName = CResolve_I__MthisName__bool_ptr[Vgen->type](Vgen->ptr, 0);
  COutput__X__CGroup *VnewOuts;
  VnewOuts = COutput__X__CGroup__Fcopy__1(Aouts);
  VnewOuts->Vout = VstructOut;
  VnewOuts->VvarOut = VstructOut;
  Zbool VdeclOutWriting;
  VdeclOutWriting = VnewOuts->VdeclOut->Vwriting;
  Zbool VbodyOutWriting;
  VbodyOutWriting = VnewOuts->VbodyOut->Vwriting;
  if (((((VclassSym->Vattributes) & 1)) && ((*(Zenum *)(Vgen->ptr + CResolve_I__VtargetLang_off[Vgen->type])) == 1)))
  {
    VnewOuts->VdeclOut->Vwriting = 0;
    VnewOuts->VbodyOut->Vwriting = 0;
  }
  Vundef += MGenerate__Fgenerate(VclassScope->VtopNode, VclassScope, Vgen, VnewOuts);
  VnewOuts->VdeclOut->Vwriting = VdeclOutWriting;
  VnewOuts->VbodyOut->Vwriting = VbodyOutWriting;
  CSContext *VnewCtx;
  VnewCtx = CSContext__FNEW(VclassScope, Vgen, VnewOuts->Vout);
  if ((((Vout->Vwriting && (VclassSym->VparentClass != NULL)) && (VclassSym->VparentClass->VmemberList != NULL)) && !((((VclassSym->Vattributes) & 1)))))
  {
    {
      Zfor_T *Zf = ZforNew(VclassSym->VparentClass->VmemberList, 2);
      CSymbol *Vs;
      for (ZforGetPtr(Zf, (char **)&Vs); ZforCont(Zf); ZforNextPtr(Zf, (char **)&Vs)) {
        if ((((Vs->Vtype == 111) || (Vs->Vtype == 34)) || (Vs->Vtype == 35)))
        {
          CSymbol *Vf;
          Vf = CSymbol__FfindMatchingFunction(VclassSym, Vs->Vname, Vs->VmemberList, NULL, 0, 0);
          if (((Vf != NULL) && (Vf->Vtlang != (*(Zenum *)(Vgen->ptr + CResolve_I__VtargetLang_off[Vgen->type])))))
          {
            CSymbol__FremoveMember(VclassSym, Vf);
            Vf = NULL;
          }
          if ((Vf == NULL))
          {
            if (((Vs->Vtype == 111) || ((*(Zenum *)(Vgen->ptr + CResolve_I__VtargetLang_off[Vgen->type])) == 2)))
            {
              CSymbol *Vscopy;
              Vscopy = CSymbol__Fcopy(Vs);
              Vscopy->Vclass = VclassSym;
              CSymbol__FaddMember__1(VclassSym, Vscopy);
            }
          else if (((Vs->Vnode != NULL) && (((Vs->Vtype == 34) || (Vs->Vtype == 35)))))
            {
              Zint VorigPass;
              VorigPass = VclassScope->Vpass;
              VclassScope->Vpass = 0;
              CNode *VsymNode;
              VsymNode = Vs->Vnode;
              Vundef += MGenerate__FgenerateOneNode(&(VsymNode), VnewCtx, VnewOuts);
              VclassScope->Vpass = VorigPass;
            }
          }
        }
      }
    }
  }
  VclassSym->VmemberList = VclassScope->VmemberList;
  if (!(((((*(Zenum *)(Vgen->ptr + CResolve_I__VtargetLang_off[Vgen->type])) == 1) && (((VclassSym->Vattributes) & 1))))))
  {
    CResolve_I__MwriteClassDecl__CSymbol__COutput__X__CGroup__COutput_ptr[Vgen->type](Vgen->ptr, VclassSym, Aouts, VstructOut);
  }
  VclassSym->VstructOut = VstructOut;
  if ((!((((VclassSym->Vattributes) & 1))) && (VclassSym->VmemberList != NULL)))
  {
    {
      Zfor_T *Zf = ZforNew(VclassSym->VmemberList, 2);
      CSymbol *Vs;
      for (ZforGetPtr(Zf, (char **)&Vs); ZforCont(Zf); ZforNextPtr(Zf, (char **)&Vs)) {
        if ((((Vs->Vattributes) & 1)))
        {
          CNode__Ferror(Anode, "non-abstract class cannot have an abstract method");
          if ((Vs->Vnode != NULL))
          {
            CNode__Ferror(Vs->Vnode, "location of the abstract method");
          }
        }
      }
    }
  }
  if (((VclassSym->Vinterfaces != NULL) && MGenerate__FdoError(Vout)))
  {
    {
      Zfor_T *Zf = ZforNew(VclassSym->Vinterfaces, 2);
      CSymbol *Vitf;
      for (ZforGetPtr(Zf, (char **)&Vitf); ZforCont(Zf); ZforNextPtr(Zf, (char **)&Vitf)) {
        CListHead *VmemberList;
        VmemberList = Vitf->VmemberList;
        if (((Vitf->Vtype == 25) && (Vitf->Vclass != NULL)))
        {
          VmemberList = Vitf->Vclass->VmemberList;
        }
        if ((VmemberList != NULL))
        {
          {
            Zfor_T *Zf = ZforNew(VmemberList, 2);
            CSymbol *Vmember;
            for (ZforGetPtr(Zf, (char **)&Vmember); ZforCont(Zf); ZforNextPtr(Zf, (char **)&Vmember)) {
              if ((Vmember->VclassName != NULL))
              {
                if ((((Vmember->Vtype == 34) || (Vmember->Vtype == 111)) || (Vmember->Vtype == 35)))
                {
                  if ((CSymbol__FfindMatchingFunction(VclassSym, Vmember->Vname, Vmember->VmemberList, NULL, 0, 0) == NULL))
                  {
                    MGenerate__Ferror(Zconcat(Zconcat("Missing implementation for ", Vmember->Vname), MGenerate__FargTypesAsString(Vmember->VmemberList)), Vitf->Vnode);
                  }
                }
              else
                {
                  if ((CSymbol__FfindMember(VclassSym, Vmember->Vname) == NULL))
                  {
                    MGenerate__Ferror(Zconcat("Missing implementation for ", Vmember->Vname), Vitf->Vnode);
                  }
                }
              }
            }
          }
        }
      }
    }
  }
  Anode->Vn_undefined = Vundef;
  return Vundef;
}
Zint MGenerate__FgenerateShared(CNode *Anode, CSContext *Actx, COutput__X__CGroup *Aouts) {
  COutput *Vout;
  Vout = Actx->Vout;
  Zint Vundef;
  Vundef = 0;
  CScope *Vscope;
  Vscope = Actx->Vscope;
  if ((!(CScope__FisClassScope(Vscope)) || Vscope->Vstatements))
  {
    CNode__Ferror(Anode, "SHARED misplaced");
  }
  if ((Vscope->Vpass > 1))
  {
    if (((MGenerate__Vskip_zero_undefined && !(MGenerate__FdoError(Vout))) && (Anode->Vn_undefined == 0)))
    {
      return 0;
    }
  }
  CScope *VsharedScope;
  VsharedScope = CScope__X__FnewScope(Vscope, 1);
  COutput__X__CGroup *VnewOuts;
  VnewOuts = COutput__X__CGroup__Fcopy__1(Aouts);
  VnewOuts->Vout = VnewOuts->VdeclOut;
  VnewOuts->VvarOut = VnewOuts->VdeclOut;
  CSContext *VnewCtx;
  VnewCtx = CSContext__FNEW(VsharedScope, Actx->Vgen, VnewOuts->Vout);
  VsharedScope->Vstatements = 0;
  VsharedScope->VinsideShared = 1;
  VsharedScope->VnodeType = 22;
  VsharedScope->Vinit = 1;
  VsharedScope->Vdepth = 0;
  VsharedScope->VmemberList = Vscope->VmemberList;
  VsharedScope->VscopeName = Zconcat(Vscope->VscopeName, "__X");
  CNode *Vsnode;
  Vsnode = Anode->Vn_left;
  while ((Vsnode != NULL))
  {
    Vundef += MGenerate__FgenerateOneNode(&(Vsnode), VnewCtx, VnewOuts);
    Vsnode = Vsnode->Vn_next;
  }
  Anode->Vn_undefined = Vundef;
  return Vundef;
}
Zint MGenerate__FgenerateBits(CNode *Anode, CSContext *Actx, COutput__X__CGroup *Aouts) {
  COutput *Vout;
  Vout = Actx->Vout;
  Zint Vundef;
  Vundef = 0;
  Zoref *Vgen;
  Vgen = Actx->Vgen;
  CScope *Vscope;
  Vscope = Actx->Vscope;
  CNode__FcheckTypeName(Anode, "bits");
  char *Vname;
  Vname = Anode->Vn_string;
  CSymbol *Vsym;
  Vsym = NULL;
  if ((Vscope->Vpass <= 1))
  {
    Vsym = CScope__FaddSymbol(Vscope, Vname, 26, Anode, 0);
    Vsym->Vclass = Vsym;
    if ((Vscope->Vpass == 1))
    {
      Anode->Vn_symbol = Vsym;
    }
    if ((Vscope->VscopeName == NULL))
    {
      Vsym->VcName = Zconcat("B", Vname);
    }
  else
    {
      Vsym->VcName = Zconcat(Zconcat(Vscope->VscopeName, "__B"), Vname);
    }
  }
else
  {
    Vsym = Anode->Vn_symbol;
    if (!(Vscope->VforwardDeclare))
    {
      CScope__FaddMember(Vscope, Vsym);
    }
  }
  if ((Vsym->VmemberList != NULL))
  {
    ZListClear(Vsym->VmemberList);
  }
  Zint Vn;
  Vn = 0;
  CNode *Ven;
  Ven = Anode->Vn_left;
  while ((Ven != NULL))
  {
    if ((Ven->Vn_type != 66))
    {
      CNode__Ferror(Ven, "Syntax error in BITS declaration");
    }
  else
    {
      Zint Vbits;
      Vbits = 0;
      CSymbol *VtypeSym;
      VtypeSym = NULL;
      char *VtypeName;
      VtypeName = Ven->Vn_left->Vn_string;
      if (((Zstrcmp(VtypeName, "Bool") == 0) || (Zstrcmp(VtypeName, "bool") == 0)))
      {
        VtypeSym = CSymbol__FNEW(11);
        Vbits = 1;
      }
    else if (((Zstrcmp(ZStringByteSlice(VtypeName, 0, 2), "int") == 0) || (Zstrcmp(ZStringByteSlice(VtypeName, 0, 2), "nat") == 0)))
      {
        if (((VtypeName[0] == 73) || (VtypeName[0] == 105)))
        {
          VtypeSym = CSymbol__FNEW(7);
        }
      else
        {
          VtypeSym = CSymbol__FNEW(8);
        }
        Vbits = ZStringToInt(ZStringByteSlice(Ven->Vn_left->Vn_string, 3, -(1)));
        if ((Vbits > 32))
        {
          CNode__Ferror(Ven->Vn_left, "Cannot have more than 32 bits");
        }
      }
    else
      {
        Ven->Vn_left->Vn_undefined = 0;
        VtypeSym = CScope__FfindNodeSymbol(Vscope, Ven->Vn_left);
        Vundef += Ven->Vn_left->Vn_undefined;
        if (((VtypeSym == NULL) || (VtypeSym->Vtype != 29)))
        {
          ++(Vundef);
          if (MGenerate__FdoError(Vout))
          {
            CNode__Ferror(Ven, Zconcat(Zconcat("Type ", Ven->Vn_left->Vn_string), " not supported"));
          }
        }
      else
        {
          Vbits = 1;
          Zint Vsize;
          Vsize = VtypeSym->VmemberList->itemCount;
          while ((Vsize > 2))
          {
            Vsize = (Vsize >> 1);
            ++(Vbits);
          }
        }
      }
      if ((Vbits > 0))
      {
        if ((CSymbol__FfindMember(Vsym, Ven->Vn_string) != NULL))
        {
          CNode__Ferror(Ven, Zconcat("Duplicate member: ", Ven->Vn_string));
        }
      else
        {
          CSymbol *Vmsym;
          Vmsym = CSymbol__FaddMember(Vsym, Ven->Vn_string, VtypeSym, Vn);
          Vmsym->Vmask = (((1 << Vbits)) - 1);
        }
        Vn += Vbits;
      }
    }
    Ven = Ven->Vn_next;
  }
  if ((Vn > MConfig__VintSize))
  {
    Vsym->Vtype = 27;
  }
  return Vundef;
}
Zint MGenerate__FgenerateEnum(CNode *Anode, CSContext *Actx, COutput__X__CGroup *Aouts) {
  CScope *Vscope;
  Vscope = Actx->Vscope;
  CNode__FcheckTypeName(Anode, "enum");
  char *Vname;
  Vname = Anode->Vn_string;
  CSymbol *Vsym;
  Vsym = CScope__FaddSymbol(Vscope, Vname, 29, Anode, 0);
  Vsym->Vclass = Vsym;
  if ((Vscope->Vpass >= 1))
  {
    Anode->Vn_symbol = Vsym;
  }
  if ((Vscope->VscopeName == NULL))
  {
    Vsym->VcName = Zconcat("E", Vname);
  }
else
  {
    Vsym->VcName = Zconcat(Zconcat(Vscope->VscopeName, "__E"), Vname);
  }
  COutput__Fwrite(Aouts->VstructOut, Zconcat(Zconcat("char *", Vsym->VcName), "[] = {\n"));
  Zint Vn;
  Vn = 0;
  CNode *Ven;
  Ven = Anode->Vn_left;
  while ((Ven != NULL))
  {
    if ((Vscope->Vpass <= 1))
    {
      CNode__FcheckItemName(Ven, "enum value");
      if ((CSymbol__FfindMember(Vsym, Ven->Vn_string) != NULL))
      {
        MGenerate__Ferror(Zconcat("Duplicate ENUM value: ", Ven->Vn_string), Ven);
      }
    else
      {
        CSymbol__FaddMember(Vsym, Ven->Vn_string, CSymbol__FNEW(30), (Vn)++);
      }
    }
    COutput__Fwrite(Aouts->VstructOut, Zconcat(Zconcat("\"", Ven->Vn_string), "\",\n"));
    Ven = Ven->Vn_left;
  }
  COutput__Fwrite(Aouts->VstructOut, "};\n");
  return 0;
}
Zint MGenerate__FgenerateFor(CNode *Anode, CSContext *Actx, COutput__X__CGroup *Aouts) {
  COutput *Vout;
  Vout = Actx->Vout;
  Zint Vundef;
  Vundef = 0;
  Zoref *Vgen;
  Vgen = Actx->Vgen;
  CScope *Vscope;
  Vscope = Actx->Vscope;
  CScope__FaddUsedItem(Vscope, "for");
  COutput__FwriteIndent(Vout, Vscope->Vdepth);
  COutput__Fwrite(Vout, "{\n");
  COutput__FwriteIndent(Vout, (Vscope->Vdepth + 1));
  CResolve_I__MforStart__COutput_ptr[Vgen->type](Vgen->ptr, Vout);
  CSymbol *ViterSym;
  ViterSym = MGenerate__FgenExpr(Anode->Vn_cond, Actx, NULL);
  Vundef = Anode->Vn_cond->Vn_undefined;
  CSymbol *VitemSym;
  VitemSym = NULL;
  if ((ViterSym != NULL))
  {
    COutput__Fwrite(Vout, ", ");
    if ((ViterSym != NULL))
    {
      switch (ViterSym->Vtype)
      {
      case 9:
        {
          {
            COutput__Fwrite(Vout, "1");
            VitemSym = CSymbol__FNEW(9);
          }
            break;
        }
      case 13:
        {
          {
            COutput__Fwrite(Vout, "2");
            if ((ViterSym->VreturnSymbol != NULL))
            {
              VitemSym = CSymbol__FcopyObject(ViterSym->VreturnSymbol);
            }
          }
            break;
        }
      case 29:
        {
          {
            COutput__Fwrite(Vout, "3");
            VitemSym = CSymbol__FNEW(7);
          }
            break;
        }
      case 0:
      default:
        {
          {
            if (MGenerate__FdoError(Vout))
            {
              CNode__Ferror(Anode->Vn_cond, Zconcat("Unsupported type: ", Zenum2string(CNode__X__EType, sizeof(CNode__X__EType) / sizeof(char *), ViterSym->Vtype)));
            }
          }
            break;
        }
      }
    }
    COutput__Fwrite(Vout, ");\n");
  }
  if ((Anode->Vn_left->Vn_type != 5))
  {
    MGenerate__Ferror("FOR loop variable must be a name", Anode);
    return 1;
  }
  char *VloopVar;
  VloopVar = Anode->Vn_left->Vn_string;
  Zbool VneedDeclare;
  VneedDeclare = 0;
  CSymbol *VvarSym;
  VvarSym = CScope__FgetSymbol__2(Vscope, VloopVar, Anode->Vn_left);
  if ((VvarSym == NULL))
  {
    VneedDeclare = 1;
    VvarSym = VitemSym;
    if ((VvarSym == NULL))
    {
      Vundef += 2;
      VvarSym = CSymbol__FNEW(0);
    }
  else
    {
      if ((VvarSym->Vtype == 0))
      {
        ++(Vundef);
      }
      VvarSym->VcName = Zconcat("V", VloopVar);
      COutput__FwriteIndent(Vout, (Vscope->Vdepth + 1));
      CResolve_I__Mvardecl__CSContext__COutput_ptr[Vgen->type](Vgen->ptr, Actx, Vout);
      CResolve_I__Mvartype__CSymbol__CNode__CScope__COutput_ptr[Vgen->type](Vgen->ptr, VvarSym, Anode, NULL, Vout);
      COutput__Fwrite(Vout, VvarSym->VcName);
      COutput__Fwrite(Vout, ";\n");
    }
  }
else if ((VitemSym != NULL))
  {
    if ((VvarSym->Vtype != VitemSym->Vtype))
    {
      ++(Vundef);
      if (MGenerate__FdoError(Vout))
      {
        MGenerate__FtypeError(VitemSym->Vtype, VvarSym->Vtype, Anode->Vn_left);
      }
    }
  }
  COutput__FwriteIndent(Vout, (Vscope->Vdepth + 1));
  CResolve_I__MforLoop__CSymbol__COutput_ptr[Vgen->type](Vgen->ptr, VvarSym, Vout);
  CScope *VforScope;
  VforScope = CScope__X__FnewScope(Vscope, 0);
  if ((Vscope->Vpass <= 1))
  {
    if ((Vscope->VscopeName == NULL))
    {
      Anode->Vn_scopeName = Zconcat("BL_", Zint2string((MGenerate__VscopeNumber)++));
    }
  else
    {
      Anode->Vn_scopeName = Zconcat(Zconcat(Vscope->VscopeName, "__"), Zint2string((MGenerate__VscopeNumber)++));
    }
  }
  if ((((((Vscope->Vpass > 1) && MGenerate__Vskip_zero_undefined) && !(MGenerate__FdoError(Vout))) && (Anode->Vn_undefined == 0)) && (Vundef == 0)))
  {
    return 0;
  }
  if (VneedDeclare)
  {
    CSymbol *VscopeSym;
    VscopeSym = CScope__FaddSymbol__1(VforScope, VloopVar, VvarSym, Anode->Vn_left, 0);
    VscopeSym->VcName = VvarSym->VcName;
  }
  ++(VforScope->Vdepth);
  VforScope->VscopeName = Anode->Vn_scopeName;
  VforScope->VtopNode = Anode->Vn_right;
  VforScope->VnodeType = 49;
  COutput__X__CGroup *VnewOuts;
  VnewOuts = COutput__X__CGroup__Fcopy__1(Aouts);
  VnewOuts->VvarOut = COutput__Fcopy(VnewOuts->Vout);
  Vundef += MGenerate__Fgenerate(VforScope->VtopNode, VforScope, Vgen, VnewOuts);
  Anode->Vn_undefined = Vundef;
  COutput__FwriteIndent(Vout, (Vscope->Vdepth + 1));
  COutput__Fwrite(Vout, "}\n");
  COutput__FwriteIndent(Vout, Vscope->Vdepth);
  COutput__Fwrite(Vout, "}\n");
  return Vundef;
}
Zint MGenerate__FgenerateReturnExit(CNode *Anode, CSContext *Actx, COutput__X__CGroup *Aouts) {
  COutput *Vout;
  Vout = Actx->Vout;
  Zint Vundef;
  Vundef = 0;
  CScope *Vscope;
  Vscope = Actx->Vscope;
  char *Vname;
  Vname = NULL;
  COutput__FwriteIndent(Vout, Vscope->Vdepth);
  if ((Anode->Vn_type == 71))
  {
    COutput__Fwrite(Vout, "return ");
    Vname = "RETURN";
  }
else
  {
    COutput__Fwrite(Vout, "exit(");
    Vname = "EXIT";
  }
  CNode *Vleft;
  Vleft = Anode->Vn_left;
  if (((Vleft != NULL) && (Vleft->Vn_type != 0)))
  {
    if (((Anode->Vn_type == 71) && (((Vscope->VreturnSymbol == NULL) || Vscope->VinsideNew))))
    {
      Vundef = 5;
      if (MGenerate__FdoError(Vout))
      {
        MGenerate__Ferror(Zconcat(Vname, " argument unexpected"), Anode);
      }
    }
  else if ((Anode->Vn_type == 72))
    {
      MGenerate__FgenExpr(Anode->Vn_left, Actx, CSymbol__X__Vint);
      Vundef = Anode->Vn_left->Vn_undefined;
    }
  else
    {
      MGenerate__FgenExpr(Anode->Vn_left, Actx, Vscope->VreturnSymbol);
      Vundef = Anode->Vn_left->Vn_undefined;
    }
  }
else
  {
    if (((Anode->Vn_type == 71) && Vscope->VinsideNew))
    {
      COutput__Fwrite(Vout, Vscope->VthisName);
    }
  else if ((Vscope->VreturnSymbol != NULL))
    {
      MGenerate__Ferror("argument expected", Anode);
    }
  }
  if ((Anode->Vn_type == 71))
  {
    COutput__Fwrite(Vout, ";\n");
  }
else
  {
    COutput__Fwrite(Vout, ");\n");
  }
  if ((((((Anode->Vn_next != NULL) && (Anode->Vn_next->Vn_type != 64)) && (Anode->Vn_next->Vn_type != 65)) && (Anode->Vn_next->Vn_type != 47)) && (Anode->Vn_next->Vn_type != 46)))
  {
    MGenerate__Ferror(Zconcat("code after ", Vname), Anode);
  }
  return Vundef;
}
Zint MGenerate__FgenerateSwitch(CNode *Anode, CSContext *Actx, COutput__X__CGroup *Aouts) {
  COutput *Vout;
  Vout = Actx->Vout;
  Zint Vundef;
  Vundef = 0;
  CScope *Vscope;
  Vscope = Actx->Vscope;
  COutput__FwriteIndent(Vout, Vscope->Vdepth);
  COutput__Fwrite(Vout, "switch (");
  CSymbol *VswitchSymbol;
  VswitchSymbol = MGenerate__FgenExpr(Anode->Vn_cond, Actx, NULL);
  Vundef = Anode->Vn_cond->Vn_undefined;
  COutput__Fwrite(Vout, ")\n");
  if ((((VswitchSymbol != NULL) && (VswitchSymbol->Vtype != 7)) && (VswitchSymbol->Vtype != 29)))
  {
    ++(Vundef);
    if (MGenerate__FdoError(Vout))
    {
      MGenerate__Ferror(Zconcat("SWITCH type must be int or enum, found ", Zenum2string(CNode__X__EType, sizeof(CNode__X__EType) / sizeof(char *), VswitchSymbol->Vtype)), Anode);
    }
  }
  Vundef += MGenerate__FgenerateScope(Anode->Vn_right, Actx, 63, VswitchSymbol, Aouts);
  return Vundef;
}
Zint MGenerate__FgenerateCase(CNode * *Rref, CSContext *Actx, COutput__X__CGroup *Aouts) {
  COutput *Vout;
  Vout = Actx->Vout;
  CNode *Vnode;
  Vnode = (*Rref);
  Zoref *Vgen;
  Vgen = Actx->Vgen;
  Zint Vundef;
  Vundef = 0;
  CScope *Vscope;
  Vscope = Actx->Vscope;
  if ((Vscope->VnodeType != 63))
  {
    MGenerate__Ferror("Not inside a SWITCH", Vnode);
  }
  if ((Vnode->Vn_type == 64))
  {
    COutput__FwriteIndent(Vout, (Vscope->Vdepth - 1));
    COutput__Fwrite(Vout, "case ");
    COutput *VcaseOut;
    VcaseOut = COutput__FNEW(COutput__X__CFragmentHead__FNEW__1());
    VcaseOut->Vwriting = MGenerate__FdoError(Vout);
    MGenerate__FgenExpr(Vnode->Vn_left, CSContext__FNEW(Vscope, Vgen, VcaseOut), Vscope->VswitchSymbol);
    Vundef = Vnode->Vn_left->Vn_undefined;
    if (MGenerate__FdoError(Vout))
    {
      COutput__Fappend(Vout, VcaseOut);
      COutput__Fwrite(Vout, ":\n");
      if ((Vscope->VcaseList != NULL))
      {
        char *Vvalue;
        Vvalue = COutput__FtoString(VcaseOut);
        {
          Zfor_T *Zf = ZforNew(Vscope->VcaseList, 2);
          char *Vs;
          for (ZforGetPtr(Zf, (char **)&Vs); ZforCont(Zf); ZforNextPtr(Zf, (char **)&Vs)) {
            if ((Zstrcmp(Vvalue, Vs) == 0))
            {
              MGenerate__Ferror("duplicate case value", Vnode);
              break;
            }
          }
        }
        ZListAdd(Vscope->VcaseList, -1, 0, Vvalue, 1);
      }
    }
  }
else
  {
    COutput__FwriteIndent(Vout, (Vscope->Vdepth - 1));
    COutput__Fwrite(Vout, "default:\n");
  }
  if ((Vnode->Vn_next == NULL))
  {
    COutput__FwriteIndent(Vout, Vscope->Vdepth);
    COutput__Fwrite(Vout, "break;\n");
  }
else if (((Vnode->Vn_next->Vn_type != 64) && (Vnode->Vn_next->Vn_type != 65)))
  {
    COutput__FwriteIndent(Vout, Vscope->Vdepth);
    COutput__Fwrite(Vout, "{\n");
    if ((Vscope->Vpass <= 1))
    {
      if ((Vscope->VscopeName == NULL))
      {
        Vnode->Vn_scopeName = Zconcat("BL_", Zint2string((MGenerate__VscopeNumber)++));
      }
    else
      {
        Vnode->Vn_scopeName = Zconcat(Zconcat(Vscope->VscopeName, "__"), Zint2string((MGenerate__VscopeNumber)++));
      }
    }
    CScope *VblockScope;
    VblockScope = CScope__X__FnewScope(Vscope, 0);
    VblockScope->VscopeName = Vnode->Vn_scopeName;
    COutput__Freset(Aouts->VvarOut, Vout);
    CSContext *VblockCtx;
    VblockCtx = CSContext__FNEW(VblockScope, Vgen, Aouts->Vout);
    while ((((((*Rref)->Vn_next != NULL) && ((*Rref)->Vn_next->Vn_type != 64)) && ((*Rref)->Vn_next->Vn_type != 65)) && ((*Rref)->Vn_next->Vn_type != 53)))
    {
      (*Rref) = (*Rref)->Vn_next;
      Vundef += MGenerate__FgenerateOneNode(Rref, VblockCtx, Aouts);
    }
    if ((((*Rref)->Vn_next != NULL) && ((*Rref)->Vn_next->Vn_type == 53)))
    {
      (*Rref) = (*Rref)->Vn_next;
      if (((((*Rref)->Vn_next != NULL) && ((*Rref)->Vn_next->Vn_type != 64)) && ((*Rref)->Vn_next->Vn_type != 65)))
      {
        CNode__Ferror((*Rref)->Vn_next, "Statement after PROCEED");
      }
    }
  else
    {
      COutput__FwriteIndent(Vout, (Vscope->Vdepth + 2));
      COutput__Fwrite(Vout, "break;\n");
    }
    COutput__FwriteIndent(Vout, Vscope->Vdepth);
    COutput__Fwrite(Vout, "}\n");
  }
  return Vundef;
}
CSymbol *MGenerate__FgenerateCall(CNode *Am_node, CSContext *Actx, CSymbol *AdestSym, Zbool AstrictType) {
  Zenum Vdest_type;
  Vdest_type = (((AdestSym == NULL))) ? (0) : (AdestSym->Vtype);
  if ((Vdest_type == 21))
  {
    Vdest_type = 24;
  }
  CSymbol *Vret;
  Vret = CSymbol__FNEW(Vdest_type);
  Am_node->Vn_undefined = 0;
  if ((Am_node->Vn_type != 107))
  {
    MGenerate__Ferror("expected method node", Am_node);
    return NULL;
  }
  CNode *Vf_node;
  Vf_node = Am_node->Vn_left;
  if ((Vf_node->Vn_type == 5))
  {
    char *VfuncName;
    VfuncName = Vf_node->Vn_string;
    Zenum Vtype;
    Vtype = CScope__FgetAType(Actx->Vscope, VfuncName);
    if ((((Vtype == 35) || (Vtype == 34)) || (Vtype == 111)))
    {
      CListHead *VargTypeList;
      VargTypeList = MGenerate__FgetSymbolListFromArgNode(Am_node->Vn_right, Actx, 0);
      Zbool Vconvert;
      Vconvert = 0;
      CSymbol *Vfunc;
      Vfunc = CScope__FfindMatchingFunc(Actx->Vscope, VfuncName, VargTypeList, NULL, 1, 0);
      if ((Vfunc == NULL))
      {
        Vconvert = 1;
        Vfunc = CScope__FfindMatchingFunc(Actx->Vscope, VfuncName, VargTypeList, NULL, 1, 1);
      }
      if ((Vfunc == NULL))
      {
        Am_node->Vn_undefined = 3;
        if (MGenerate__FdoError(Actx->Vout))
        {
          MGenerate__Ferror(Zconcat(Zconcat("No function with matching arguments for ", VfuncName), "()"), Am_node);
          MGenerate__FlistMatchingFunc(Actx->Vscope, VfuncName, VargTypeList, 1);
        }
      }
    else
      {
        CSymbol *Vother;
        Vother = CScope__FfindMatchingFunc(Actx->Vscope, VfuncName, VargTypeList, Vfunc, 1, Vconvert);
        if ((Vother != NULL))
        {
          Am_node->Vn_undefined = 2;
          if (MGenerate__FdoError(Actx->Vout))
          {
            MGenerate__Ferror(Zconcat(Zconcat("More than one function with matching arguments for ", VfuncName), "()"), Am_node);
            MGenerate__FlistMatchingFunc(Actx->Vscope, VfuncName, VargTypeList, 0);
          }
        }
      else
        {
          Vret = MGenerate__FgenerateFunctionCall(Vfunc, Am_node, Actx, Vdest_type);
        }
      }
    }
  else if (((Vtype == 37) || (Vtype == 36)))
    {
      CSymbol *Vsym;
      Vsym = CScope__FgetSymbol__2(Actx->Vscope, VfuncName, Vf_node);
      if ((Vsym == NULL))
      {
        Am_node->Vn_undefined = 2;
      }
    else
      {
        MGenerate__FgenerateRefCast(Vsym, Vf_node, Actx->Vout);
        CResolve_I__MwriteSymName__CSymbol__CSContext_ptr[Actx->Vgen->type](Actx->Vgen->ptr, Vsym, Actx);
        COutput__Fwrite(Actx->Vout, ")(");
        MGenerate__FgenerateArgumentsCheck(Am_node->Vn_right, VfuncName, Actx, Vsym->VmemberList);
        Am_node->Vn_undefined = Am_node->Vn_right->Vn_undefined;
        COutput__Fwrite(Actx->Vout, ")");
        Vret = Vsym->VreturnSymbol;
      }
    }
  else
    {
      Am_node->Vn_undefined = 4;
      if (MGenerate__FdoError(Actx->Vout))
      {
        MGenerate__Ferror(Zconcat("unknown function: ", VfuncName), Am_node);
      }
    }
  }
else if ((Vf_node->Vn_type == 105))
  {
    CNode *Vleft;
    Vleft = Vf_node->Vn_left;
    CSymbol *VmoduleSym;
    VmoduleSym = NULL;
    VmoduleSym = CScope__FfindNodeSymbol(Actx->Vscope, Vleft);
    if (((VmoduleSym == NULL) || (VmoduleSym->Vtype != 21)))
    {
      VmoduleSym = CNode__FfindTopModule(Vleft, Actx->Vscope);
    }
    if ((VmoduleSym != NULL))
    {
      VmoduleSym = CScope__FfindNodeSymbol(Actx->Vscope, Vleft);
      if ((VmoduleSym == NULL))
      {
        Am_node->Vn_undefined = 3;
        if (MGenerate__FdoError(Actx->Vout))
        {
          MGenerate__Ferror(Zconcat("Cannot find module member: ", Vleft->Vn_string), Vleft);
        }
      }
    else if ((Zstrcmp(Vf_node->Vn_string, "INFO") == 0))
      {
        if (((Am_node->Vn_right != NULL) && (Am_node->Vn_right->Vn_type != 0)))
        {
          CNode__Ferror(Am_node, "INFO() does not take arguments");
        }
      else
        {
          Vret = MGenerate__FgenerateModuleInfo(VmoduleSym, Am_node, Actx);
        }
      }
    else
      {
        CSymbol *Vsym;
        Vsym = CSymbol__X__Ffind(VmoduleSym->VmemberList, Vf_node->Vn_string);
        if (((Vsym != NULL) && CSymbol__FhasMember(Vsym)))
        {
          Vsym = MGenerate__FfindMethod(VmoduleSym, Vf_node->Vn_string, Am_node->Vn_right, Actx, 0, (MGenerate__FdoError(Actx->Vout)) ? (Vf_node) : (NULL), "");
        }
        if ((Vsym == NULL))
        {
          Am_node->Vn_undefined = 1;
          if (MGenerate__FdoError(Actx->Vout))
          {
            MGenerate__Ferror(Zconcat("module member not found: ", Vf_node->Vn_string), Vleft);
          }
        }
      else if (((Vsym->Vtype == 34) || (Vsym->Vtype == 35)))
        {
          Vret = MGenerate__FgenerateFunctionCall(Vsym, Am_node, Actx, Vdest_type);
        }
      else
        {
          Vret = MGenerate__FgenerateModuleCall(Vsym, Am_node->Vn_right, Actx);
          Am_node->Vn_undefined += Am_node->Vn_right->Vn_undefined;
        }
      }
    }
  else
    {
      Vret = MGenerate__FgenerateMethodCall(Am_node, Actx, AdestSym);
    }
  }
else
  {
    MGenerate__Ferror("INTERNAL: unimplemented function call", Vf_node);
  }
  return Vret;
}
Zint MGenerate__FgenerateIncDec(CNode *Anode, CSContext *Actx, COutput__X__CGroup *Aouts) {
  COutput *Vout;
  Vout = Actx->Vout;
  CScope *Vscope;
  Vscope = Actx->Vscope;
  COutput__FwriteIndent(Vout, Vscope->Vdepth);
  if ((Anode->Vn_type == 69))
  {
    COutput__Fwrite(Vout, "++(");
  }
else
  {
    COutput__Fwrite(Vout, "--(");
  }
  CSymbol *Vsym;
  Vsym = MGenerate__FgenerateLVarname(Anode->Vn_left, 1, Actx, CSymbol__FNEW(7));
  COutput__Fwrite(Vout, ");\n");
  if (((((Anode->Vn_left->Vn_type == 5) && (Vsym != NULL)) && (Vsym->VcName != NULL)) && (Vsym->VcName[0] == 65)))
  {
    CNode__Ferror(Anode, "++/-- for argument not allowed");
  }
  return Anode->Vn_left->Vn_undefined;
}
Zint MGenerate__FgenerateAssign(CNode *Anode, CSContext *Actx, COutput__X__CGroup *Aouts) {
  COutput *Vout;
  Vout = Actx->Vout;
  CScope *Vscope;
  Vscope = Actx->Vscope;
  Zoref *Vgen;
  Vgen = Actx->Vgen;
  Zint Vundef;
  Vundef = 0;
  COutput__FwriteIndent(Vout, Vscope->Vdepth);
  if ((Anode->Vn_left->Vn_type == 108))
  {
    CSymbol *Vsp;
    Vsp = MGenerate__FgenerateVarname(Anode->Vn_left->Vn_left, CSContext__FNEW(Vscope, Vgen, MGenerate__VnoOut), NULL);
    if (((Vsp != NULL) && (Vsp->Vtype == 14)))
    {
      return MGenerate__FgenerateDictAssign(Anode, Vsp, Actx, Aouts);
    }
  }
  CSymbol *Vsp;
  Vsp = MGenerate__FgenerateLVarname(Anode->Vn_left, 1, Actx, NULL);
  Vundef += Anode->Vn_left->Vn_undefined;
  if (((Vsp != NULL) && ((((((Anode->Vn_left->Vn_type == 5) || (((Anode->Vn_left->Vn_type == 105) && (((Vsp->Vtype == 26) || (Vsp->Vtype == 27))))))) && (Vsp->VcName != NULL)) && (Vsp->VcName[0] == 65)))))
  {
    CNode__Ferror(Anode, "Assignment to argument not allowed");
  }
  Zbool VtoInterface;
  VtoInterface = 0;
  CSymbol *VexprSym;
  VexprSym = NULL;
  if (((Vsp != NULL) && (Vsp->Vtype == 25)))
  {
    CSymbol *Vs;
    Vs = MGenerate__FgenExpr(Anode->Vn_right, CSContext__FcopyNoOut(Actx), Vsp);
    if (((Vs != NULL) && (Vs->Vtype != 25)))
    {
      VtoInterface = 1;
    }
  }
  switch (Anode->Vn_type)
  {
  case 54:
    {
      {
        COutput__Fwrite(Vout, " = ");
      }
        break;
    }
  case 55:
    {
      {
        if (((Vsp != NULL) && (Vsp->Vtype == 7)))
        {
          COutput__Fwrite(Vout, " -= ");
        }
      else
        {
          ++(Vundef);
          if (MGenerate__FdoError(Vout))
          {
            CNode__Ferror(Anode, Zconcat("-= not supported for ", Zenum2string(CNode__X__EType, sizeof(CNode__X__EType) / sizeof(char *), Vsp->Vtype)));
          }
        }
      }
        break;
    }
  case 56:
    {
      {
        if (((Vsp != NULL) && (Vsp->Vtype == 7)))
        {
          COutput__Fwrite(Vout, " += ");
        }
      else
        {
          ++(Vundef);
          if (MGenerate__FdoError(Vout))
          {
            CNode__Ferror(Anode, Zconcat("+= not supported for ", Zenum2string(CNode__X__EType, sizeof(CNode__X__EType) / sizeof(char *), Vsp->Vtype)));
          }
        }
      }
        break;
    }
  case 57:
    {
      {
        if (((Vsp != NULL) && (Vsp->Vtype == 9)))
        {
          CScope__FaddUsedItem(Actx->Vscope, "concat");
          COutput__Fwrite(Vout, " = Zconcat(");
          MGenerate__FgenerateVarname(Anode->Vn_left, Actx, CSymbol__FNEW(9));
          Vundef += Anode->Vn_left->Vn_undefined;
          COutput__Fwrite(Vout, ", ");
        }
      else
        {
          ++(Vundef);
          if (MGenerate__FdoError(Vout))
          {
            CNode__Ferror(Anode, Zconcat("..= not supported for ", Zenum2string(CNode__X__EType, sizeof(CNode__X__EType) / sizeof(char *), Vsp->Vtype)));
          }
        }
      }
        break;
    }
  }
  if ((Vsp != NULL))
  {
    if ((Vsp->Vtype == 18))
    {
      CSymbol *Vr;
      Vr = MGenerate__FgenExpr(Anode->Vn_right, Actx, NULL);
      if (((Vr == NULL) || (Vr->Vtype == 0)))
      {
        ++(Vundef);
        if (MGenerate__FdoError(Vout))
        {
          MGenerate__Ferror(Zconcat("Cannot determine type for VAR variable ", Vsp->Vname), Anode);
        }
        MError__Fverbose2Msg(Zconcat(Anode->Vn_string, " VAR type not detected\n"));
        Vsp->Vtype = 0;
      }
    else
      {
        Vsp->Vtype = Vr->Vtype;
        Vsp->Vclass = Vr->Vclass;
        Vsp->VmemberList = Vr->VmemberList;
        Vsp->VcName = Vr->VcName;
        Vsp->VkeySymbol = Vr->VkeySymbol;
        Vsp->VreturnSymbol = Vr->VreturnSymbol;
      }
    }
  else if (((((Vsp->Vtype == 26) || (Vsp->Vtype == 27))) && (Vsp->VreturnSymbol != NULL)))
    {
      COutput__Fwrite(Vout, "((");
      MGenerate__FgenerateVarname(Anode->Vn_left, Actx, NULL);
      Vundef += Anode->Vn_left->Vn_undefined;
      Zint Vshift;
      Vshift = Vsp->VreturnSymbol->Vvalue;
      COutput__Fwrite(Vout, Zconcat(Zconcat(") & ", Zint2string(~(((Vsp->VreturnSymbol->Vmask << Vshift))))), ") | (("));
      CSymbol *VretSym;
      VretSym = Vsp->VreturnSymbol;
      if ((Vsp->VreturnSymbol->Vtype == 8))
      {
        VretSym = CSymbol__Fcopy(Vsp->VreturnSymbol);
        VretSym->Vtype = 7;
      }
      MGenerate__FgenExpr(Anode->Vn_right, Actx, VretSym);
      if ((Vshift == 0))
      {
        COutput__Fwrite(Vout, "))");
      }
    else
      {
        COutput__Fwrite(Vout, Zconcat(Zconcat(") << ", Zint2string(Vshift)), ")"));
      }
    }
  else if ((Vsp->Vtype == 25))
    {
      if (VtoInterface)
      {
        COutput__Fwrite(Vout, "(void *)");
        VexprSym = MGenerate__FgenExpr(Anode->Vn_right, Actx, NULL);
      }
    else
      {
        MGenerate__FgenExpr(Anode->Vn_right, Actx, Vsp);
      }
    }
  else if ((Vsp->Vtype == 6))
    {
      MGenerate__FgenExpr(Anode->Vn_right, Actx, Vsp->VreturnSymbol);
    }
  else
    {
      MGenerate__FgenExpr(Anode->Vn_right, Actx, Vsp);
    }
    Vundef += Anode->Vn_right->Vn_undefined;
    if (((Anode->Vn_type == 57) && (Vsp->Vtype == 9)))
    {
      COutput__Fwrite(Vout, ")");
    }
  }
  if (((((Vsp != NULL) && (Vsp->Vclass != NULL)) && (VexprSym != NULL)) && (VexprSym->Vclass != NULL)))
  {
    COutput__Fwrite(Vout, "; ");
    MGenerate__FgenerateVarname(Anode->Vn_left, Actx, NULL);
    Vundef += Anode->Vn_left->Vn_undefined;
    COutput__Fwrite(Vout, "->type = ");
    Zint Vidx;
    Vidx = CSymbol__FchildIndex(Vsp->Vclass, VexprSym->Vclass);
    if ((Vidx < 0))
    {
      if (Vout->Vwriting)
      {
        CNode__Ferror(Anode->Vn_right, "Type mismatch, expression result does not match the destination class");
      }
    else
      {
        ++(Vundef);
      }
    }
    COutput__Fwrite(Vout, Zconcat("", Zint2string(Vidx)));
  }
  COutput__Fwrite(Vout, ";\n");
  return Vundef;
}
Zint MGenerate__FgenerateDictAssign(CNode *Anode, CSymbol *Asp, CSContext *Actx, COutput__X__CGroup *Aouts) {
  COutput *Vout;
  Vout = Actx->Vout;
  Zint Vundef;
  Vundef = 0;
  if ((Anode->Vn_type != 54))
  {
    CNode__Ferror(Anode, "Sorry, only \"=\" supported for dict");
  }
else
  {
    COutput__Fwrite(Vout, "ZDictAdd(1, ");
    MGenerate__FgenerateVarname(Anode->Vn_left->Vn_left, Actx, NULL);
    Vundef = Anode->Vn_left->Vn_left->Vn_undefined;
    if (CSymbol__FisPointerType(Asp->VkeySymbol))
    {
      COutput__Fwrite(Vout, ", 0, ");
    }
  else
    {
      COutput__Fwrite(Vout, ", ");
    }
    MGenerate__FgenExpr(Anode->Vn_left->Vn_right, Actx, Asp->VkeySymbol);
    Vundef += Anode->Vn_left->Vn_right->Vn_undefined;
    if (!(CSymbol__FisPointerType(Asp->VkeySymbol)))
    {
      COutput__Fwrite(Vout, ", NULL");
    }
    COutput__Fwrite(Vout, ", ");
    if ((Asp->VreturnSymbol == NULL))
    {
      Vundef += 3;
    }
  else
    {
      if (CSymbol__FisPointerType(Asp->VreturnSymbol))
      {
        COutput__Fwrite(Vout, "0, ");
      }
      MGenerate__FgenExpr(Anode->Vn_right, Actx, Asp->VreturnSymbol);
      Vundef += Anode->Vn_right->Vn_undefined;
      if (!(CSymbol__FisPointerType(Asp->VreturnSymbol)))
      {
        COutput__Fwrite(Vout, ", NULL");
      }
    }
    COutput__Fwrite(Vout, ");\n");
  }
  return Vundef;
}
Zint MGenerate__FgenerateMethod(CNode *Anode, CSContext *Actx, COutput__X__CGroup *Aouts) {
  COutput *Vout;
  Vout = Actx->Vout;
  Zint Vundef;
  Vundef = 0;
  Zoref *Vgen;
  Vgen = Actx->Vgen;
  CScope *Vscope;
  Vscope = Actx->Vscope;
  CSymbol *VretSym;
  VretSym = NULL;
  Zbool VisEqual;
  VisEqual = ((Anode->Vn_type == 32));
  Zbool VisNew;
  VisNew = ((Anode->Vn_type == 33));
  char *Vname;
  Vname = Anode->Vn_string;
  if ((VisNew || VisEqual))
  {
    if (!(CScope__FisClassScope(Vscope)))
    {
      MGenerate__Ferror(Zconcat(Vname, " not in class scope"), Anode);
      return 0;
    }
    Vname = (VisNew) ? ("NEW") : ("EQUAL");
  }
else
  {
    CNode__FcheckItemName(Anode, ((Anode->Vn_type == 35)) ? ("proc") : ("func"));
  }
  CSymbol *Vsym;
  Vsym = NULL;
  if ((Vscope->Vpass <= 1))
  {
    Zint VidentNumber;
    VidentNumber = 0;
    Vsym = CScope__FgetSymbol__1(Vscope, Vname, 0);
    if ((Vsym != NULL))
    {
      if (((((Vsym->Vtype != 34) && (Vsym->Vtype != 35)) && (Vsym->Vtype != 111)) && (Vsym->Vtype != Anode->Vn_type)))
      {
        CNode__Ferror(Anode, Zconcat(Zconcat(Zconcat("Symbol \"", Vname), "\" redefined as "), Zenum2string(CNode__X__EType, sizeof(CNode__X__EType) / sizeof(char *), Vsym->Vtype)));
        if ((Vsym->Vnode != NULL))
        {
          CNode__Ferror(Vsym->Vnode, "Previous definition here");
        }
      }
    else
      {
        VidentNumber = (Vsym->Vvalue + 1);
      }
    }
    Vsym = CScope__FaddSymbol(Vscope, Vname, Anode->Vn_type, Anode, 1);
    Vsym->Vvalue = VidentNumber;
    if ((Vscope->Vpass == 1))
    {
      Anode->Vn_symbol = Vsym;
    }
  }
else
  {
    Vsym = Anode->Vn_symbol;
    if (!(Vscope->VforwardDeclare))
    {
      CScope__FaddMember(Vscope, Vsym);
    }
    if (((MGenerate__Vskip_zero_undefined && !(MGenerate__FdoError(Vout))) && (Anode->Vn_undefined == 0)))
    {
      return 0;
    }
  }
  Vsym->Vnode = Anode;
  Vsym->Vattributes = Anode->Vn_attr;
  Vsym->Vtlang = (*(Zenum *)(Vgen->ptr + CResolve_I__VtargetLang_off[Vgen->type]));
  if (CScope__FisClassScope(Vscope))
  {
    Vsym->VclassName = Vscope->Vclass->VclassName;
    if ((Vscope->VscopeName == NULL))
    {
      Vsym->VcName = Zconcat(Zconcat(Zconcat("M", Vsym->VclassName), "__"), Vname);
    }
  else
    {
      Vsym->VcName = Zconcat(Zconcat(Vscope->VscopeName, "__F"), Vname);
    }
    if ((Vsym->Vvalue > 0))
    {
      Vsym->VcName = Zconcat(Vsym->VcName, Zconcat("__", Zint2string(Vsym->Vvalue)));
    }
  }
else
  {
    if ((Vscope->VscopeName == NULL))
    {
      Vsym->VcName = Zconcat("F", Vname);
    }
  else
    {
      Vsym->VcName = Zconcat(Zconcat(Vscope->VscopeName, "__F"), Vname);
    }
    if ((Vsym->Vvalue > 0))
    {
      Vsym->VcName = Zconcat(Vsym->VcName, Zconcat("__", Zint2string(Vsym->Vvalue)));
    }
  }
  COutput *VfuncOut;
  VfuncOut = NULL;
  if ((((Anode->Vn_attr) & 1)))
  {
    VfuncOut = MGenerate__VnoOut;
  }
else
  {
    VfuncOut = COutput__FNEW(COutput__X__CFragmentHead__FNEW__1());
    VfuncOut->Vwriting = Vout->Vwriting;
  }
  CSContext *VfuncOutCtx;
  VfuncOutCtx = CSContext__FNEW(Vscope, Vgen, VfuncOut);
  VretSym = CResolve_I__MmethodReturnType__CNode__CSymbol__CSContext_ptr[Vgen->type](Vgen->ptr, Anode, Vsym, VfuncOutCtx);
  if (((VretSym == NULL) && (Anode->Vn_type != 35)))
  {
    ++(Vundef);
    if ((MGenerate__FdoError(Vout) && (Anode->Vn_type == 34)))
    {
      CNode__Ferror(Anode->Vn_returnType, "Unknown type");
      VretSym = CSymbol__X__Vnil;
    }
  else
    {
      MError__Fverbose2Msg(Zconcat(Vname, " return type undefined\n"));
    }
  }
  if (((VretSym != NULL) && (VretSym->Vtype == 21)))
  {
    VretSym = CSymbol__FcopyObject(VretSym);
  }
  Vsym->VreturnSymbol = VretSym;
  CScope *VfuncScope;
  VfuncScope = NULL;
  if (!((((Anode->Vn_attr) & 1))))
  {
    VfuncScope = CScope__X__FnewScope(Vscope, 0);
    VfuncScope->VscopeName = Vsym->VcName;
    VfuncScope->VnodeType = 35;
  }
  Vsym->VmemberList = NULL;
  CNode *Varg_node;
  Varg_node = Anode->Vn_left;
  while ((Varg_node != NULL))
  {
    CNode *Vleft;
    Vleft = Varg_node->Vn_left;
    CSymbol *VtypeSym;
    VtypeSym = MGenerate__FgenerateObjDeclType(Vleft, CSContext__FNEW(Vscope, Vgen, MGenerate__VnoOut));
    if ((VtypeSym == NULL))
    {
      ++(Vundef);
      MError__Fverbose2Msg(Zconcat(Zconcat(Zconcat(Vname, " arg "), Varg_node->Vn_string), " type undefined\n"));
      if (MGenerate__FdoError(Vout))
      {
        MGenerate__Ferror(Zconcat("Unknown type: ", Vleft->Vn_string), Vleft);
      }
    }
  else if ((((VtypeSym->Vtype == 6) && (VtypeSym->VreturnSymbol != NULL)) && (VtypeSym->VreturnSymbol->Vtype == 21)))
    {
      VtypeSym->VreturnSymbol = CSymbol__FcopyObject(VtypeSym->VreturnSymbol);
    }
    CSymbol__FaddMember(Vsym, Varg_node->Vn_string, VtypeSym, 0);
    if ((VfuncScope != NULL))
    {
      CSymbol *VargSym;
      VargSym = NULL;
      if ((VtypeSym == NULL))
      {
        VargSym = CScope__FaddSymbol(VfuncScope, Varg_node->Vn_string, 0, Varg_node, 0);
      }
    else
      {
        VargSym = CScope__FaddSymbol__1(VfuncScope, Varg_node->Vn_string, VtypeSym, Varg_node, 0);
      }
      char *VargName;
      VargName = NULL;
      if (((VtypeSym != NULL) && (VtypeSym->Vtype == 6)))
      {
        VargName = Zconcat("R", Varg_node->Vn_string);
        if ((VargSym->VreturnSymbol != NULL))
        {
          VargSym->VreturnSymbol = CSymbol__Fcopy(VargSym->VreturnSymbol);
          VargSym->VreturnSymbol->VcName = VargName;
          VargSym->VreturnSymbol->VclassName = NULL;
        }
      }
    else
      {
        VargName = Zconcat("A", Varg_node->Vn_string);
        VargSym->VcName = VargName;
      }
      if ((MGenerate__FdoError(Vout) && (VtypeSym != NULL)))
      {
        CResolve_I__MargWithType__bool__CSymbol__CNode__string__CSContext_ptr[Vgen->type](Vgen->ptr, (Varg_node == Anode->Vn_left), VtypeSym, Vleft, VargName, VfuncOutCtx);
      }
    }
    Varg_node = Varg_node->Vn_next;
  }
  if ((MGenerate__FdoError(Vout) && (Vscope->Vpass >= 1)))
  {
    if ((CScope__FfindMatchingFunc(Vscope, Vsym->Vname, Vsym->VmemberList, Vsym, 0, 0) != NULL))
    {
      CNode__Ferror(Anode, Zconcat(Zconcat("Redefining ", Vsym->Vname), " with same signature"));
    }
  else if ((Vscope->Vclass != NULL))
    {
      CSymbol *Vparent;
      Vparent = Vscope->Vclass->VparentClass;
      while ((Vparent != NULL))
      {
        CSymbol *VotherSym;
        VotherSym = CSymbol__FfindMatchingFunction(Vparent, Vsym->Vname, Vsym->VmemberList, NULL, 0, 0);
        if ((VotherSym != NULL))
        {
          if (((((VotherSym->Vattributes) & 2)) && !((((Vsym->Vattributes) & 16)))))
          {
            CNode__Ferror(Anode, Zconcat(Zconcat("Replacing default method ", Vsym->Vname), " without REPLACE"));
            CNode__Ferror(VotherSym->Vnode, "Previously defined method is here");
          }
        else if (((((VotherSym->Vattributes) & 1)) && !((((Vsym->Vattributes) & 8)))))
          {
            CNode__Ferror(Anode, Zconcat(Zconcat("Defining abstract method ", Vsym->Vname), " without DEFINE"));
            CNode__Ferror(VotherSym->Vnode, "Previously defined method is here");
          }
        else if ((!((((VotherSym->Vattributes) & 1))) && !((((VotherSym->Vattributes) & 2)))))
          {
            CNode__Ferror(Anode, Zconcat("Cannot redefine ", Vsym->Vname));
            CNode__Ferror(VotherSym->Vnode, Zconcat("Previously defined method ", "is not ABSTRACT or VIRTUAL"));
          }
        }
        Vparent = Vparent->VparentClass;
      }
    }
  }
  if ((((Anode->Vn_attr) & 1)))
  {
    Anode->Vn_undefined = Vundef;
    return Vundef;
  }
  if ((CResolve_I__MdoWriteDecl_ptr[Vgen->type](Vgen->ptr) && (VfuncOut != MGenerate__VnoOut)))
  {
    COutput__Fappend(Aouts->VdeclOut, VfuncOut);
    COutput__Fwrite(Aouts->VdeclOut, ");\n");
  }
  COutput__Fwrite(VfuncOut, ") {\n");
  if (VisNew)
  {
    CResolve_I__MwriteNewThis__CSymbol__CSContext_ptr[Vgen->type](Vgen->ptr, Vsym, VfuncOutCtx);
  }
  VfuncScope->Vdepth = 1;
  VfuncScope->VreturnSymbol = VretSym;
  VfuncScope->VinsideNew = VisNew;
  VfuncScope->VtopNode = Anode->Vn_right;
  VfuncScope->VthisName = CResolve_I__MthisName__bool_ptr[Vgen->type](Vgen->ptr, VisNew);
  COutput__X__CGroup *VnewOuts;
  VnewOuts = COutput__X__CGroup__Fcopy__1(Aouts);
  if ((Aouts->VorigBodyOut == NULL))
  {
    VnewOuts->VorigBodyOut = Aouts->VbodyOut;
  }
  VnewOuts->VbodyOut = VfuncOut;
  VnewOuts->VvarOut = COutput__Fcopy(VnewOuts->VbodyOut);
  VnewOuts->Vout = VfuncOut;
  Vundef += MGenerate__Fgenerate(VfuncScope->VtopNode, VfuncScope, Vgen, VnewOuts);
  if (VisNew)
  {
    CResolve_I__MwriteNewReturn__COutput_ptr[Vgen->type](Vgen->ptr, VfuncOut);
  }
else if ((Anode->Vn_type == 34))
  {
    CNode *Vn;
    Vn = Anode->Vn_right;
    while (1)
    {
      if (((Vn == NULL) || (((Vn->Vn_next == NULL) && (Vn->Vn_type != 71)))))
      {
        CNode__Ferror(Anode, "Missing RETURN");
        break;
      }
      Vn = Vn->Vn_next;
      if ((Vn == NULL))
      {
        break;
      }
    }
  }
  COutput__Fwrite(VfuncOut, "}\n");
  if ((Aouts->VorigBodyOut != NULL))
  {
    COutput__Fappend(Aouts->VorigBodyOut, VfuncOut);
  }
else
  {
    COutput__Fappend(Aouts->VbodyOut, VfuncOut);
  }
  Anode->Vn_undefined = Vundef;
  return Vundef;
}
Zint MGenerate__FgenerateDeclare(CNode *Anode, CSContext *Actx, COutput__X__CGroup *Aouts) {
  COutput *Vout;
  Vout = Actx->Vout;
  COutput *VvarOut;
  VvarOut = Aouts->VvarOut;
  Zint Vundef;
  Vundef = 0;
  Zoref *Vgen;
  Vgen = Actx->Vgen;
  CScope *Vscope;
  Vscope = Actx->Vscope;
  CSymbol *VtypeSym;
  VtypeSym = NULL;
  CNode__FcheckItemName(Anode, "member");
  if ((Vscope->Vouter != NULL))
  {
    COutput__FwriteIndent(VvarOut, Vscope->Vdepth);
  }
  if ((Anode->Vn_left->Vn_type == 18))
  {
    if ((Vscope->Vpass <= 1))
    {
      VtypeSym = CSymbol__FNEW(18);
      ++(Vundef);
      MError__Fverbose2Msg(Zconcat(Anode->Vn_string, " is a VAR, undefined\n"));
    }
  else
    {
      VtypeSym = Anode->Vn_symbol;
      if ((VtypeSym->Vtype == 18))
      {
        ++(Vundef);
        MError__Fverbose2Msg(Zconcat(Anode->Vn_string, " VAR type not yet detected\n"));
        if (MGenerate__FdoError(Vout))
        {
          MGenerate__Ferror(Zconcat("Could not determine type for ", Anode->Vn_string), Anode);
        }
      }
    else
      {
        if ((((VtypeSym->Vtype == 21) || (VtypeSym->Vtype == 24)) || (VtypeSym->Vtype == 29)))
        {
          VtypeSym = VtypeSym->Vclass;
        }
        CResolve_I__Mvartype__CSymbol__CNode__CScope__COutput_ptr[Vgen->type](Vgen->ptr, VtypeSym, Anode->Vn_left, Vscope, VvarOut);
      }
    }
  }
else
  {
    VtypeSym = MGenerate__FgenerateDeclType(Anode->Vn_left, CSContext__FNEW(Vscope, Vgen, VvarOut));
    Vundef += Anode->Vn_left->Vn_undefined;
  }
  CResolve_I__Mvardecl__CSContext__COutput_ptr[Vgen->type](Vgen->ptr, Actx, VvarOut);
  if (((VtypeSym == NULL) || (VtypeSym->Vtype == 0)))
  {
    VtypeSym = CSymbol__FNEW(0);
  }
  CSymbol *Vsym;
  Vsym = NULL;
  switch (VtypeSym->Vtype)
  {
  case 21:
    {
      {
        CSymbol *VobjSym;
        VobjSym = CSymbol__FcopyObject(VtypeSym);
        Vsym = CScope__FaddSymbol__1(Vscope, Anode->Vn_string, VobjSym, Anode, 0);
      }
        break;
    }
  case 24:
  case 25:
  case 7:
  case 9:
  case 11:
  case 12:
  case 26:
  case 27:
  case 29:
  case 18:
  case 13:
  case 14:
  case 37:
  case 36:
    {
      {
        Vsym = CScope__FaddSymbol__1(Vscope, Anode->Vn_string, VtypeSym, Anode, 0);
      }
        break;
    }
  default:
    {
      {
        Vsym = CScope__FaddSymbol__1(Vscope, Anode->Vn_string, VtypeSym, Anode, 0);
        ++(Vundef);
        if (MGenerate__FdoError(Vout))
        {
          MGenerate__Ferror(Zconcat(Zconcat(Zconcat("Declaration of ", Anode->Vn_string), " for unsupported type "), Zenum2string(CNode__X__EType, sizeof(CNode__X__EType) / sizeof(char *), VtypeSym->Vtype)), Anode);
        }
      }
        break;
    }
  }
  if (((CScope__FisClassScope(Vscope) || (Vscope->VscopeName == NULL)) || Vscope->Vstatements))
  {
    Vsym->VcName = Zconcat("V", Anode->Vn_string);
  }
else
  {
    Vsym->VcName = Zconcat(Zconcat(Vscope->VscopeName, "__V"), Anode->Vn_string);
  }
  if ((Vscope->Vpass >= 1))
  {
    Anode->Vn_symbol = Vsym;
  }
  CNode *Vanode;
  Vanode = Anode->Vn_left;
  while ((Vanode != NULL))
  {
    if ((Vanode->Vn_type == 108))
    {
      Vsym->VreturnSymbol = CSymbol__FNEW(Vsym->Vtype);
      Vsym->Vtype = 16;
      break;
    }
    Vanode = Vanode->Vn_left;
  }
  if (((Vanode != NULL) && CSymbol__FisPointerType(VtypeSym)))
  {
    COutput__Fwrite(VvarOut, "(");
  }
  COutput__Fwrite(VvarOut, Vsym->VcName);
  if ((Vanode != NULL))
  {
    COutput__Fwrite(VvarOut, "[");
    MGenerate__FgenExpr(Vanode->Vn_right, CSContext__FNEW(Vscope, Vgen, VvarOut), CSymbol__X__Vint);
    COutput__Fwrite(VvarOut, "]");
    Vundef += Vanode->Vn_right->Vn_undefined;
    if (CSymbol__FisPointerType(VtypeSym))
    {
      COutput__Fwrite(VvarOut, ")");
    }
  }
  COutput__Fwrite(VvarOut, ";\n");
  if (Vscope->Vinit)
  {
    COutput *ViOut;
    ViOut = Vout;
    if (((*(Zenum *)(Vgen->ptr + CResolve_I__VtargetLang_off[Vgen->type])) == 2))
    {
      ViOut = VvarOut;
      COutput__FwriteIndent(ViOut, Vscope->Vdepth);
    }
  else if ((((Vscope->VscopeName == NULL) || (Vscope->VmoduleName != NULL)) || (Vscope->VnodeType == 22)))
    {
      ViOut = Aouts->VinitOut;
      COutput__FwriteIndent(ViOut, 1);
    }
  else
    {
      COutput__FwriteIndent(ViOut, Vscope->Vdepth);
    }
    COutput__Fwrite(ViOut, Vsym->VcName);
    COutput__Fwrite(ViOut, " = ");
    if ((Anode->Vn_right == NULL))
    {
      CResolve_I__MdefaultInit__CSymbol__COutput_ptr[Vgen->type](Vgen->ptr, VtypeSym, ViOut);
    }
  else
    {
      Zbool VvarType;
      VvarType = 0;
      if ((VtypeSym->Vtype == 18))
      {
        VtypeSym->Vtype = 0;
        VvarType = 1;
      }
      if ((VtypeSym->Vtype == 21))
      {
        VtypeSym = CSymbol__FcopyObject(VtypeSym);
      }
      CSymbol *VexprSym;
      VexprSym = MGenerate__FgenExpr(Anode->Vn_right, CSContext__FNEW(Vscope, Vgen, ViOut), VtypeSym);
      Vundef += Anode->Vn_right->Vn_undefined;
      if (VvarType)
      {
        if ((VexprSym->Vtype == 0))
        {
          Vsym->Vtype = 18;
        }
      else
        {
          --(Vundef);
          MError__Fverbose2Msg(Zconcat(Anode->Vn_string, " VAR type detected\n"));
          Vsym->Vtype = VexprSym->Vtype;
          Vsym->VmemberList = VexprSym->VmemberList;
          Vsym->Vclass = VexprSym->Vclass;
          Vsym->VkeySymbol = VexprSym->VkeySymbol;
          Vsym->VreturnSymbol = VexprSym->VreturnSymbol;
        }
      }
    }
    COutput__Fwrite(ViOut, ";\n");
  }
  return Vundef;
}
Zint MGenerate__FgenerateGenerateIf(CNode *Anode, CSContext *Actx, COutput__X__CGroup *Aouts) {
  CNode *Vgen_node;
  Vgen_node = Anode->Vn_left;
  Zint Vundef;
  Vundef = 0;
  Zbool VdidBlock;
  VdidBlock = 0;
  Zbool VnoGenerate_save;
  VnoGenerate_save = Actx->Vscope->VnoGenerate;
  COutput__X__CGroup *VnoOuts;
  VnoOuts = COutput__X__CGroup__FNEW__1();
  COutput__X__CGroup__FsetHeads(VnoOuts, COutput__X__CHeads__FNEW__1());
  while ((Vgen_node != NULL))
  {
    Zbool VdoGenerate;
    VdoGenerate = 0;
    if (!(Actx->Vout->Vwriting))
    {
      VdoGenerate = 1;
    }
  else if (VdidBlock)
    {
      VdoGenerate = 0;
    }
  else if ((Vgen_node->Vn_type == 44))
    {
      VdoGenerate = 1;
    }
  else
    {
      VdoGenerate = MExprEval__FevalBool(Vgen_node->Vn_cond, Actx);
    }
    Vgen_node = Vgen_node->Vn_next;
    if ((Vgen_node->Vn_type != 58))
    {
      CNode__Ferror(Vgen_node, "INTERNAL: expected block node");
      break;
    }
    Actx->Vscope->VnoGenerate = !(VdoGenerate);
    Vundef += MGenerate__FgenerateNode(Vgen_node->Vn_left, Actx->Vscope, Actx->Vgen, (VdoGenerate) ? (Aouts) : (VnoOuts));
    if (VdoGenerate)
    {
      VdidBlock = 1;
    }
    Vgen_node = Vgen_node->Vn_next;
  }
  Actx->Vscope->VnoGenerate = VnoGenerate_save;
  return Vundef;
}
Zint MGenerate__FhandleImplements(CNode *Anode, CSymbol *Aclass, CScope *Ascope, Zbool AgiveError) {
  Zint Vundef;
  Vundef = 0;
  if ((Anode->Vn_type == 110))
  {
    Vundef += MGenerate__FhandleImplements(Anode->Vn_left, Aclass, Ascope, AgiveError);
    Vundef += MGenerate__FhandleImplements(Anode->Vn_right, Aclass, Ascope, AgiveError);
  }
else
  {
    CSymbol *Vitf;
    Vitf = CScope__FfindNodeSymbol(Ascope, Anode);
    if ((Vitf == NULL))
    {
      Vundef += 2;
      if (AgiveError)
      {
        CNode__Ferror(Anode, "Interface not found");
      }
    }
  else if (((Vitf->Vtype != 23) && (Vitf->Vtype != 25)))
    {
      ++(Vundef);
      if (AgiveError)
      {
        CNode__Ferror(Anode, "Not an interface");
      }
    }
    if ((Vitf != NULL))
    {
      Vitf->Vnode = Anode;
      CSymbol__FaddInterface(Aclass, Vitf, Anode);
      if ((Vitf->Vtype == 25))
      {
        CSymbol__FaddChild(Vitf->Vclass, Aclass);
      }
    else
      {
        CSymbol__FaddChild(Vitf, Aclass);
      }
    }
  }
  return Vundef;
}
Zint MGenerate__FgenerateScope(CNode *Anode, CSContext *Actx, Zenum AnodeType, CSymbol *AswitchSymbol, COutput__X__CGroup *Aouts) {
  COutput *Vout;
  Vout = Aouts->Vout;
  COutput__FwriteIndent(Vout, Actx->Vscope->Vdepth);
  COutput__Fwrite(Vout, "{\n");
  Zint Vundef;
  Vundef = 0;
  if ((Anode != NULL))
  {
    CScope *VblockScope;
    VblockScope = CScope__X__FnewScope(Actx->Vscope, 0);
    if ((Actx->Vscope->Vpass <= 1))
    {
      if ((Actx->Vscope->VscopeName == NULL))
      {
        Anode->Vn_scopeName = Zconcat("BL_", Zint2string((MGenerate__VscopeNumber)++));
      }
    else
      {
        Anode->Vn_scopeName = Zconcat(Zconcat(Actx->Vscope->VscopeName, "__"), Zint2string((MGenerate__VscopeNumber)++));
      }
    }
    if (((((Actx->Vscope->Vpass > 1) && MGenerate__Vskip_zero_undefined) && !(MGenerate__FdoError(Vout))) && (Anode->Vn_undefined == 0)))
    {
      return 0;
    }
    VblockScope->VscopeName = Anode->Vn_scopeName;
    VblockScope->VtopNode = Anode;
    VblockScope->VnodeType = AnodeType;
    if ((AnodeType == 63))
    {
      VblockScope->VswitchSymbol = AswitchSymbol;
      VblockScope->VcaseList = Zalloc(sizeof(CListHead));
    }
    COutput__X__CGroup *VnewOuts;
    VnewOuts = COutput__X__CGroup__Fcopy__1(Aouts);
    VnewOuts->VvarOut = COutput__Fcopy(VnewOuts->Vout);
    Vundef = MGenerate__Fgenerate(VblockScope->VtopNode, VblockScope, Actx->Vgen, VnewOuts);
    Anode->Vn_undefined = Vundef;
  }
  COutput__FwriteIndent(Vout, Actx->Vscope->Vdepth);
  COutput__Fwrite(Vout, "}\n");
  return Vundef;
}
void MGenerate__FgenerateClassOffTable(CSymbol *AclassSym, CSContext *Actx) {
  if ((AclassSym->Vchildren == NULL))
  {
    return ;
  }
  CScope__FaddUsedItem(Actx->Vscope, "dummy");
  MGenerate__FgenClassOffTableList(AclassSym, AclassSym, Actx->Vout, 1);
  CSymbol *Vsym;
  Vsym = AclassSym->VparentClass;
  while ((Vsym != NULL))
  {
    MGenerate__FgenClassOffTableList(AclassSym, Vsym, Actx->Vout, 0);
    Vsym = Vsym->VparentClass;
  }
}
void MGenerate__FgenClassOffTableList(CSymbol *AclassSym, CSymbol *AloopSym, COutput *Aout, Zbool AdoMethods) {
  if ((AloopSym->VmemberList == NULL))
  {
    return ;
  }
  {
    Zfor_T *Zf = ZforNew(AloopSym->VmemberList, 2);
    CSymbol *Vm;
    for (ZforGetPtr(Zf, (char **)&Vm); ZforCont(Zf); ZforNextPtr(Zf, (char **)&Vm)) {
      if ((Vm->VclassName != NULL))
      {
        if (CSymbol__FisMethodType(Vm))
        {
          if (AdoMethods)
          {
            MGenerate__FgenerateMethodReturnType(Vm, Aout);
            COutput__Fwrite(Aout, Zconcat(Zconcat(" (*(", MGenerate__FclassFuncTableName(AclassSym, Vm)), "[]))("));
            MGenerate__FgenerateMethodArgTypes(Vm, Aout);
            COutput__Fwrite(Aout, ") = {\n  ");
            if (!((((AloopSym->Vattributes) & 1))))
            {
              MGenerate__FmethodTypeCast(Vm, Aout);
              COutput__Fwrite(Aout, Zconcat(Vm->VcName, ",\n"));
            }
            MGenerate__FgenMethodChildrenList(AclassSym, Vm, Aout);
            COutput__Fwrite(Aout, "};\n");
          }
        }
      else
        {
          COutput__Fwrite(Aout, Zconcat(Zconcat("int ", MGenerate__FclassOffTableName(AclassSym, Vm)), "[] = {\n"));
          if (!((((AclassSym->Vattributes) & 1))))
          {
            COutput__Fwrite(Aout, Zconcat(Zconcat("  (char *)&((", AclassSym->VcName), " *)&dummy)->"));
            COutput__Fwrite(Aout, Zconcat(Zconcat(Zconcat(Vm->VcName, " - (char *)("), AclassSym->VcName), " *)&dummy,\n"));
          }
          MGenerate__FgenMemberChildrenList(AclassSym, Vm, Aout);
          COutput__Fwrite(Aout, "};\n");
        }
      }
    }
  }
}
void MGenerate__FgenMethodChildrenList(CSymbol *Aclass, CSymbol *Amember, COutput *Aout) {
  {
    Zfor_T *Zf = ZforNew(Aclass->Vchildren, 2);
    CSymbol *Vc;
    for (ZforGetPtr(Zf, (char **)&Vc); ZforCont(Zf); ZforNextPtr(Zf, (char **)&Vc)) {
      CSymbol *Vf;
      Vf = CSymbol__FfindMatchingMember(Vc, Amember);
      if ((Vf == NULL))
      {
        COutput__Fwrite(Aout, "  NULL,\n");
      }
    else if (!((((Vc->Vattributes) & 1))))
      {
        COutput__Fwrite(Aout, "  ");
        MGenerate__FmethodTypeCast(Amember, Aout);
        COutput__Fwrite(Aout, Zconcat(Vf->VcName, ",\n"));
      }
      if ((Vc->Vchildren != NULL))
      {
        MGenerate__FgenMethodChildrenList(Vc, Amember, Aout);
      }
    }
  }
}
void MGenerate__FgenMemberChildrenList(CSymbol *Aclass, CSymbol *Amember, COutput *Aout) {
  {
    Zfor_T *Zf = ZforNew(Aclass->Vchildren, 2);
    CSymbol *Vc;
    for (ZforGetPtr(Zf, (char **)&Vc); ZforCont(Zf); ZforNextPtr(Zf, (char **)&Vc)) {
      if (!((((Vc->Vattributes) & 1))))
      {
        COutput__Fwrite(Aout, Zconcat(Zconcat("  (char *)&((", Vc->VcName), " *)&dummy)->"));
        COutput__Fwrite(Aout, Zconcat(Zconcat(Zconcat(Amember->VcName, " - (char *)("), Vc->VcName), " *)&dummy,\n"));
      }
      if ((Vc->Vchildren != NULL))
      {
        MGenerate__FgenMemberChildrenList(Vc, Amember, Aout);
      }
    }
  }
}
void MGenerate__FmethodTypeCast(CSymbol *Am, COutput *Aout) {
  COutput__Fwrite(Aout, "(");
  MGenerate__FgenerateMethodReturnType(Am, Aout);
  COutput__Fwrite(Aout, " (*)(");
  MGenerate__FgenerateMethodArgTypes(Am, Aout);
  COutput__Fwrite(Aout, "))");
}
char *MGenerate__FclassFuncTableName(CSymbol *AclassSym, CSymbol *Amember) {
  char *Vs;
  Vs = Zconcat(Zconcat(AclassSym->VcName, "_I__M"), Amember->Vname);
  if ((Amember->VmemberList != NULL))
  {
    {
      Zfor_T *Zf = ZforNew(Amember->VmemberList, 2);
      CSymbol *Vm;
      for (ZforGetPtr(Zf, (char **)&Vm); ZforCont(Zf); ZforNextPtr(Zf, (char **)&Vm)) {
        if ((Vm != NULL))
        {
          if (((Vm->Vclass != NULL) && (((Vm->Vtype == 21) || (Vm->Vtype == 24)))))
          {
            Vs = Zconcat(Vs, Zconcat("__", Vm->Vclass->VcName));
          }
        else if (((Vm->Vclass != NULL) && (Vm->Vtype == 25)))
          {
            Vs = Zconcat(Vs, Zconcat(Zconcat("__", Vm->Vclass->VcName), "_I"));
          }
        else if (((Vm->VcName != NULL) && (Vm->Vtype == 29)))
          {
            Vs = Zconcat(Vs, Zconcat("__", Vm->VcName));
          }
        else
          {
            Vs = Zconcat(Vs, Zconcat("__", Zenum2string(CNode__X__EType, sizeof(CNode__X__EType) / sizeof(char *), Vm->Vtype)));
          }
        }
      }
    }
  }
  return Zconcat(Vs, "_ptr");
}
char *MGenerate__FclassOffTableName(CSymbol *AclassSym, CSymbol *Amember) {
  return Zconcat(Zconcat(Zconcat(AclassSym->VcName, "_I__"), Amember->VcName), "_off");
}
void MGenerate__FgenerateMethodReturnType(CSymbol *Asym, COutput *Aout) {
  if ((Zstrcmp(Asym->Vname, "NEW") == 0))
  {
    COutput__Fwrite(Aout, Zconcat(Asym->VreturnSymbol->VclassName, " *"));
  }
else if ((Zstrcmp(Asym->Vname, "EQUAL") == 0))
  {
    COutput__Fwrite(Aout, "Zbool ");
  }
else if ((Asym->Vtype == 34))
  {
    MGenerate__FgenType(Asym->VreturnSymbol, NULL, Aout);
  }
else
  {
    COutput__Fwrite(Aout, "void ");
  }
}
void MGenerate__FgenerateMethodArgTypes(CSymbol *Asym, COutput *Aout) {
  char *Vsep;
  Vsep = "";
  if ((Zstrcmp(Asym->Vname, "NEW") != 0))
  {
    COutput__Fwrite(Aout, "void *");
    Vsep = ", ";
  }
  if ((Asym->VmemberList != NULL))
  {
    {
      Zfor_T *Zf = ZforNew(Asym->VmemberList, 2);
      CSymbol *Vs;
      for (ZforGetPtr(Zf, (char **)&Vs); ZforCont(Zf); ZforNextPtr(Zf, (char **)&Vs)) {
        COutput__Fwrite(Aout, Vsep);
        MGenerate__FgenType(Vs, NULL, Aout);
        Vsep = ", ";
      }
    }
  }
}
Zint MGenerate__FgenerateImport(CUsedFile *AusedFile, CNode *Anode, Zoref *Agen, COutput__X__CGroup *Aouts) {
  CScope *Vscope;
  Vscope = CUsedFile__Fscope(AusedFile);
  if ((Anode->Vn_usedFile == NULL))
  {
    MGenerate__FhandleImport(AusedFile, Anode);
  }
  CZimbuFile *VzimbuFile;
  VzimbuFile = Anode->Vn_usedFile->VzimbuFile;
  if ((VzimbuFile == NULL))
  {
    return 0;
  }
  char *Vname;
  Vname = VzimbuFile->Vfilename;
  if ((Vscope->Vpass <= 1))
  {
    CScope__FaddImport(Vscope, VzimbuFile);
  }
  Zint Vundef;
  Vundef = 0;
  if ((VzimbuFile->VstartedPass == -(1)))
  {
    CUsedFile__Fparse(Anode->Vn_usedFile, Zconcat(Vscope->VimportIndent, "  "));
    if ((VzimbuFile->VtopScope == NULL))
    {
      CNode__Ferror(Anode, Zconcat("Cannot open file for reading: ", Vname));
    }
  else
    {
      Zint Vtail;
      Vtail = ZStringRindex(Vname, 47);
      if ((Vtail < 0))
      {
        VzimbuFile->VtopScope->VdirName = "";
      }
    else
      {
        VzimbuFile->VtopScope->VdirName = ZStringByteSlice(Vname, 0, (Vtail - 1));
      }
    }
  }
  if ((CUsedFile__FisNewImport(Anode->Vn_usedFile, VzimbuFile) && !(MError__VfoundError)))
  {
    VzimbuFile->VstartedPass = 1;
    MGenerate__Fresolve__1(Anode->Vn_usedFile, Zconcat(Vscope->VimportIndent, "  "));
    CNode *Vtop;
    Vtop = VzimbuFile->VtopScope->VtopNode;
    if ((Vtop != NULL))
    {
      Vundef = VzimbuFile->VtopScope->VtopNode->Vn_undefined;
      while (((Vtop != NULL) && (Vtop->Vn_type == 38)))
      {
        Vtop = Vtop->Vn_next;
      }
      if (((Vtop != NULL) && ((((((Vtop->Vn_type == 21) || (Vtop->Vn_type == 23)) || (Vtop->Vn_type == 19)) || (Vtop->Vn_type == 31)) || (Vtop->Vn_type == 28)))))
      {
        char *VtopName;
        VtopName = ZStringToLower(Vtop->Vn_string);
        char *Vroot;
        Vroot = NULL;
        if ((Zstrcmp(ZStringByteSlice(Vname, -(3), -(1)), ".zu") == 0))
        {
          Vroot = ZStringToLower(ZStringByteSlice(Vname, 0, -(4)));
        }
      else
        {
          Vroot = ZStringToLower(ZStringByteSlice(Vname, 0, -(5)));
        }
        Zint Vslash;
        Vslash = ZStringRindex(Vroot, 47);
        if ((Vslash >= 0))
        {
          Vroot = ZStringByteSlice(Vroot, (Vslash + 1), -(1));
        }
        if ((ZStringFind(Vroot, VtopName) < 0))
        {
          CNode__Ferror(Vtop, "Name must match part of the file name");
        }
      }
    }
  }
  if (!(MError__VfoundError))
  {
    Vundef += MGenerate__FprocessImport(Anode->Vn_usedFile, Agen, Vscope, Aouts);
  }
  return Vundef;
}
void MGenerate__FhandleImport(CUsedFile *AusedFile, CNode *Anode) {
  CScope *Vscope;
  Vscope = CUsedFile__Fscope(AusedFile);
  char *Vname;
  Vname = Anode->Vn_string;
  if ((Zstrcmp(ZStringByteSlice(Vname, -(3), -(1)), ".zu") != 0))
  {
    CNode__Ferror(Anode, Zconcat("Input name must end in '.zu': ", Vname));
    return ;
  }
  if (((Vscope->VdirName != NULL) && (Zstrcmp(Vscope->VdirName, "") != 0)))
  {
    char *Vdir;
    Vdir = Vscope->VdirName;
    while ((Zstrcmp(ZStringByteSlice(Vname, 0, 2), "../") == 0))
    {
      Vname = ZStringByteSlice(Vname, 3, -(1));
      Zint Vslash;
      Vslash = ZStringRindex(Vdir, 47);
      if ((Vslash < 0))
      {
        Vdir = "";
        break;
      }
      Vdir = ZStringByteSlice(Vdir, 0, (Vslash - 1));
    }
    if ((Zstrcmp(Vdir, "") != 0))
    {
      Vname = Zconcat(Zconcat(Vdir, "/"), Vname);
    }
  }
  CZimbuFile *VzimbuFile;
  VzimbuFile = CZimbuFile__X__Ffind(MGenerate__VimportedFiles, Vname);
  if ((VzimbuFile == NULL))
  {
    VzimbuFile = CZimbuFile__FNEW(Vname);
    ZListAdd(MGenerate__VimportedFiles, -1, 0, VzimbuFile, 1);
  }
  Zbool VtopFile;
  VtopFile = 0;
  if (((Anode->Vn_left != NULL) && (Zstrcmp(Anode->Vn_left->Vn_string, "ZWT") == 0)))
  {
    VzimbuFile->VusedAsZwt = 1;
    VzimbuFile->VtopZwtFile = 1;
    VtopFile = 1;
  }
else
  {
    if ((Anode->Vn_left != NULL))
    {
      CNode__Ferror(Anode, Zconcat("Undefined IMPORT type: ", Anode->Vn_left->Vn_string));
    }
    if (AusedFile->VzimbuFile->VusedAsZwt)
    {
      VzimbuFile->VusedAsZwt = 1;
    }
    if (AusedFile->VzimbuFile->VusedAsZimbu)
    {
      VzimbuFile->VusedAsZimbu = 1;
    }
  }
  Anode->Vn_usedFile = CUsedFile__FNEW__1(VzimbuFile, 0, VtopFile);
  Anode->Vn_usedFile->Vparent = AusedFile;
}
Zint MGenerate__FprocessImport(CUsedFile *AusedFile, Zoref *Agen, CScope *Ascope, COutput__X__CGroup *Aouts) {
  CZimbuFile *VzimbuFile;
  VzimbuFile = AusedFile->VzimbuFile;
  if ((Zstrcmp(VzimbuFile->VdirName, "") != 0))
  {
    VzimbuFile->VoutDir = Zconcat(Zconcat(VzimbuFile->VdirName, "/"), MConfig__VzudirName);
  }
else
  {
    VzimbuFile->VoutDir = MConfig__VzudirName;
  }
  char *Vname;
  Vname = VzimbuFile->Vfilename;
  Zint Vundef;
  Vundef = 0;
  if ((((VzimbuFile->VstartedPass < Ascope->Vpass) && !(MError__VfoundError)) && !(MGenerate__FdoError(Aouts->Vout))))
  {
    VzimbuFile->VstartedPass = Ascope->Vpass;
    if ((VzimbuFile->VtopScope->VtopNode != NULL))
    {
      if ((!(MGenerate__Vskip_zero_undefined) || (VzimbuFile->VtopScope->VtopNode->Vn_undefined > 0)))
      {
        MGenerate__Fresolve__1(AusedFile, Zconcat(Ascope->VimportIndent, "  "));
        Vundef = VzimbuFile->VtopScope->VtopNode->Vn_undefined;
      }
    else
      {
        MError__FverboseMsg(Zconcat(Zconcat(Zconcat(Ascope->VimportIndent, "  "), Vname), ": Skipping\n"));
        ++(VzimbuFile->VtopScope->Vpass);
      }
    }
  }
  if (((CResolve_I__MneedWrite__CZimbuFile_ptr[Agen->type](Agen->ptr, VzimbuFile) && !(MError__VfoundError)) && MGenerate__FdoError(Aouts->Vout)))
  {
    VzimbuFile->VtopScope->Vpass = Ascope->Vpass;
    if (VzimbuFile->VtopZwtFile)
    {
      MGenerate__FgenerateJSFile(AusedFile, Ascope);
    }
    MGenerate__Fwrite__1(AusedFile, Agen, CResolve_I__MgetCS__CZimbuFile_ptr[Agen->type](Agen->ptr, VzimbuFile)->Voutputs);
    if ((VzimbuFile->VtopScope->VtopNode != NULL))
    {
      Vundef = VzimbuFile->VtopScope->VtopNode->Vn_undefined;
    }
    if (((*(Zenum *)(Agen->ptr + CResolve_I__VtargetLang_off[Agen->type])) == 1))
    {
      CResolve_I__MwriteImport__CZimbuFile__COutput__X__CGroup__CScope_ptr[Agen->type](Agen->ptr, VzimbuFile, Aouts, Ascope);
    }
    if ((!(COutput__X__CFragmentHead__Fempty(CResolve_I__MgetCS__CZimbuFile_ptr[Agen->type](Agen->ptr, VzimbuFile)->Vheads->VmainLines)) && ((*(Zenum *)(Agen->ptr + CResolve_I__VtargetLang_off[Agen->type])) != 2)))
    {
      MError__Freport(Zconcat(Zconcat("Lines at toplevel of imported file ", VzimbuFile->Vfilename), "\n"));
    }
  }
  if (!(VzimbuFile->VtopZwtFile))
  {
    CScope__FmergeKeywords(Ascope, VzimbuFile->VtopScope);
  }
  if (Aouts->Vout->Vwriting)
  {
    CResolve_I__MwriteIncludeImport__CZimbuFile__COutput__X__CGroup__CScope_ptr[Agen->type](Agen->ptr, VzimbuFile, Aouts, Ascope);
  }
  return Vundef;
}
void MGenerate__FgenerateJSFile(CUsedFile *AusedFile, CScope *Ascope) {
  CNode *Vnode;
  Vnode = CZimbuFile__FgetModuleNode(AusedFile->VzimbuFile);
  if ((Vnode == NULL))
  {
    return ;
  }
  CSymbol *VmoduleSym;
  VmoduleSym = Vnode->Vn_symbol;
  VmoduleSym->VzwtPermu = ZnewDict(1);
  char *VmoduleName;
  VmoduleName = CZimbuFile__FgetModuleName(AusedFile->VzimbuFile);
  if ((VmoduleName != NULL))
  {
    char *Vpat;
    Vpat = Zconcat(Zconcat(Zconcat(AusedFile->VzimbuFile->VoutDir, "/"), VmoduleName), ".*.html");
    (fputs(Zconcat("Deleting ", Vpat), stdout) | fputc('\n', stdout));
    fflush(stdout), system(Zconcat("rm ", Vpat));
  }
  {
    Zfor_T *Zf = ZforNew(MZWTvalues__VpermuNames, 2);
    char *Vpermu;
    for (ZforGetPtr(Zf, (char **)&Vpermu); ZforCont(Zf); ZforNextPtr(Zf, (char **)&Vpermu)) {
      COutput__X__CGroup *VjsOutputs;
      VjsOutputs = COutput__X__CGroup__FNEW__1();
      COutput__X__CGroup__FsetHeads(VjsOutputs, COutput__X__CHeads__FNEW__1());
      CWrite_JS *VnewGen;
      VnewGen = CWrite_JS__FNEW();
      VnewGen->VpermuName = Vpermu;
      MGenerate__Fwrite__1(AusedFile, ZallocZoref(VnewGen, 2), VjsOutputs);
      char *Vfname;
      Vfname = CWrite_JS__FwriteImport(VnewGen, AusedFile->VzimbuFile, VjsOutputs, Ascope);
      if ((Vfname != NULL))
      {
        MIO__CStat *Vstat;
        Vstat = ZStat(Vfname);
        Zint Vi;
        Vi = ZStringRindex(Vfname, 47);
        if ((Vi < 0))
        {
          MError__Freport("INTERNAL: no slash in file name");
        }
      else
        {
          char *VnewTail;
          VnewTail = Zconcat(Zconcat(Zconcat(VmoduleName, "."), Zint2string(Vstat->size)), ".html");
          char *Vdest;
          Vdest = Zconcat(ZStringByteSlice(Vfname, 0, Vi), VnewTail);
          (fputs(Zconcat(Zconcat(Zconcat("Renaming ", Vfname), " to "), Vdest), stdout) | fputc('\n', stdout));
          rename(Vfname, Vdest);
          ZDictAdd(0, VmoduleSym->VzwtPermu, 0, Vpermu, 0, VnewTail);
        }
      }
    }
  }
}
void MGenerate__FcheckScope(CNode *Anode, CScope *Ascope, Zbool AisBreak) {
  CScope *Vs;
  Vs = Ascope;
  while ((Vs != NULL))
  {
    if (((((Vs->VnodeType == 49) || (Vs->VnodeType == 48)) || (Vs->VnodeType == 67)) || ((AisBreak && (Vs->VnodeType == 63)))))
    {
      break;
    }
    if (CNode__X__FisMethodType(Vs->VnodeType))
    {
      Vs = NULL;
      break;
    }
    Vs = Vs->Vouter;
  }
  if ((Vs == NULL))
  {
    if (AisBreak)
    {
      CNode__Ferror(Anode, "BREAK not inside FOR, WHILE, DO or SWITCH");
    }
  else
    {
      CNode__Ferror(Anode, "CONTINUE not inside FOR, DO or WHILE");
    }
  }
}
CSymbol *MGenerate__FgenerateObjDeclType(CNode *Anode, CSContext *Actx) {
  CSymbol *Vsp;
  Vsp = MGenerate__FgenerateDeclType(Anode, Actx);
  if ((Vsp != NULL))
  {
    if ((Vsp->Vtype == 21))
    {
      Vsp = CSymbol__FcopyObject(Vsp);
    }
  else if ((((Vsp->Vtype == 6) && (Vsp->VreturnSymbol != NULL)) && (Vsp->VreturnSymbol->Vtype == 21)))
    {
      Vsp->VreturnSymbol = CSymbol__FcopyObject(Vsp->VreturnSymbol);
    }
  }
  return Vsp;
}
CSymbol *MGenerate__FgenerateDeclType(CNode *Anode, CSContext *Actx) {
  CSymbol *Vsp;
  Vsp = NULL;
  Anode->Vn_undefined = 0;
  if ((Anode->Vn_type == 5))
  {
    Zenum Vtype;
    Vtype = CNode__X__Fname2Type(Anode->Vn_string);
    if ((Vtype == 0))
    {
      Vsp = CScope__FgetSymbol__2(Actx->Vscope, Anode->Vn_string, Anode);
    }
  else
    {
      Vsp = CSymbol__FNEW(Vtype);
      Vsp->Vname = Anode->Vn_string;
    }
    if ((Vsp != NULL))
    {
      if ((Vsp->Vtype == 19))
      {
        Vsp = CSymbol__FfindMember(Vsp, Anode->Vn_string);
        if ((Vsp == NULL))
        {
          ++(Anode->Vn_undefined);
          if (MGenerate__FdoError(Actx->Vout))
          {
            MGenerate__Ferror("Cannot use module; no class with this name", Anode);
          }
        }
      }
    else if ((Vsp->Vtype == 23))
      {
        CSymbol *Vs;
        Vs = CSymbol__FNEW(25);
        Vs->Vclass = Vsp;
        Vsp = Vs;
      }
    }
    if ((Vsp == NULL))
    {
      ++(Anode->Vn_undefined);
      if (MGenerate__FdoError(Actx->Vout))
      {
        MGenerate__Ferror(Zconcat("Symbol not found: ", Anode->Vn_string), Anode);
      }
    }
  else
    {
      CResolve_I__Mvartype__CSymbol__CNode__CScope__COutput_ptr[Actx->Vgen->type](Actx->Vgen->ptr, Vsp, Anode, Actx->Vscope, Actx->Vout);
    }
  }
else if ((Anode->Vn_type == 6))
  {
    Vsp = CSymbol__FNEW(6);
    Vsp->VreturnSymbol = MGenerate__FgenerateDeclType(Anode->Vn_left, Actx);
    Anode->Vn_undefined = Anode->Vn_left->Vn_undefined;
  }
else if ((Anode->Vn_type == 105))
  {
    Anode->Vn_undefined = 0;
    Vsp = CScope__FfindNodeSymbol__1(Actx->Vscope, Anode, 19, 0);
    if ((Vsp == NULL))
    {
      Anode->Vn_undefined = 0;
      Vsp = CScope__FfindNodeSymbol__1(Actx->Vscope, Anode, 0, MGenerate__FdoError(Actx->Vout));
    }
    if ((Vsp == NULL))
    {
      Anode->Vn_undefined = 1;
    }
  else
    {
      CResolve_I__Mvartype__CSymbol__CNode__CScope__COutput_ptr[Actx->Vgen->type](Actx->Vgen->ptr, Vsp, Anode, Actx->Vscope, Actx->Vout);
    }
  }
else if ((Anode->Vn_type == 108))
  {
    Vsp = MGenerate__FgenerateDeclType(Anode->Vn_left, Actx);
    Anode->Vn_undefined = Anode->Vn_left->Vn_undefined;
  }
else if ((Anode->Vn_type == 109))
  {
    Vsp = MGenerate__FgenerateContainerType(Anode, Actx);
  }
else if ((Anode->Vn_type == 107))
  {
    MGenerate__Ferror("cannot have method in type", Anode);
  }
else
  {
    MGenerate__Ferror(Zconcat("unknown type for declaration: ", Zenum2string(CNode__X__EType, sizeof(CNode__X__EType) / sizeof(char *), Anode->Vn_type)), Anode);
  }
  return Vsp;
}
CSymbol *MGenerate__FgenerateContainerType(CNode *Anode, CSContext *Actx) {
  if ((Anode->Vn_left->Vn_type != 5))
  {
    CNode__Ferror(Anode, "Expected identifier before <>");
    return NULL;
  }
  if ((Zstrcmp(Anode->Vn_left->Vn_string, "list") == 0))
  {
    if ((Anode->Vn_right->Vn_type == 110))
    {
      CNode__Ferror(Anode, "list<> takes only one type");
      return NULL;
    }
    CSymbol *Vsp;
    Vsp = CSymbol__FNEW(13);
    Vsp->VreturnSymbol = MGenerate__FgenerateObjDeclType(Anode->Vn_right, CSContext__FcopyNoOut(Actx));
    Anode->Vn_undefined = Anode->Vn_right->Vn_undefined;
    CScope__FaddUsedItem(Actx->Vscope, "list");
    CResolve_I__Mvartype__CSymbol__CNode__CScope__COutput_ptr[Actx->Vgen->type](Actx->Vgen->ptr, Vsp, Anode, Actx->Vscope, Actx->Vout);
    return Vsp;
  }
  if ((Zstrcmp(Anode->Vn_left->Vn_string, "dict") == 0))
  {
    if (((((Anode->Vn_right->Vn_type != 110) || (Anode->Vn_right->Vn_left == NULL)) || (Anode->Vn_right->Vn_left->Vn_left != NULL)) || (Anode->Vn_right->Vn_right == NULL)))
    {
      CNode__Ferror(Anode, "dict<> requires two types");
      return NULL;
    }
    CSymbol *Vsp;
    Vsp = CSymbol__FNEW(14);
    Vsp->VkeySymbol = MGenerate__FgenerateObjDeclType(Anode->Vn_right->Vn_left, CSContext__FcopyNoOut(Actx));
    Vsp->VreturnSymbol = MGenerate__FgenerateObjDeclType(Anode->Vn_right->Vn_right, CSContext__FcopyNoOut(Actx));
    Anode->Vn_undefined = (Anode->Vn_right->Vn_left->Vn_undefined + Anode->Vn_right->Vn_right->Vn_undefined);
    MDictStuff__VuseDict = 1;
    CResolve_I__Mvartype__CSymbol__CNode__CScope__COutput_ptr[Actx->Vgen->type](Actx->Vgen->ptr, Vsp, Anode, Actx->Vscope, Actx->Vout);
    return Vsp;
  }
  Zbool VisFunc;
  VisFunc = (Zstrcmp(Anode->Vn_left->Vn_string, "func") == 0);
  if ((VisFunc || (Zstrcmp(Anode->Vn_left->Vn_string, "proc") == 0)))
  {
    if ((VisFunc && (Anode->Vn_right->Vn_type == 0)))
    {
      CNode__Ferror(Anode, "func<> requires at least one type");
      return NULL;
    }
    CSymbol *Vsp;
    Vsp = NULL;
    CNode *VargNode;
    VargNode = Anode->Vn_right;
    CNode *VlastLeft;
    VlastLeft = NULL;
    if (VisFunc)
    {
      Vsp = CSymbol__FNEW(36);
      CNode *VtypeNode;
      VtypeNode = Anode->Vn_right;
      if ((Anode->Vn_right->Vn_type == 110))
      {
        do
        {
          if ((VlastLeft == NULL))
          {
            VlastLeft = Anode;
          }
        else if ((VlastLeft == Anode))
          {
            VlastLeft = Anode->Vn_right;
          }
        else
          {
            VlastLeft = VlastLeft->Vn_left;
          }
          VtypeNode = VtypeNode->Vn_left;
        }
          while (!((VtypeNode->Vn_type != 110)));
      }
    else
      {
        VargNode = CNode__FNEW(0);
      }
      Vsp->VreturnSymbol = MGenerate__FgenerateObjDeclType(VtypeNode, CSContext__FcopyNoOut(Actx));
      Anode->Vn_undefined = VtypeNode->Vn_undefined;
    }
  else
    {
      Vsp = CSymbol__FNEW(37);
    }
    CNode *VsaveLeft;
    VsaveLeft = NULL;
    if ((VlastLeft != NULL))
    {
      if ((VlastLeft == Anode))
      {
        VargNode = Anode->Vn_right->Vn_right;
      }
    else
      {
        VsaveLeft = VlastLeft->Vn_left;
        VlastLeft->Vn_left = VlastLeft->Vn_left->Vn_right;
      }
    }
    Vsp->VmemberList = MGenerate__FgetSymbolListFromArgNode(VargNode, Actx, (1 + ((MGenerate__FdoError(Actx->Vout)) ? (2) : (0))));
    if ((VlastLeft != NULL))
    {
      VlastLeft->Vn_left = VsaveLeft;
    }
    CResolve_I__Mvartype__CSymbol__CNode__CScope__COutput_ptr[Actx->Vgen->type](Actx->Vgen->ptr, Vsp, Anode, Actx->Vscope, Actx->Vout);
    return Vsp;
  }
  CNode__Ferror(Anode, "Only list<>, dict<>, proc<> and func<> supported now");
  return NULL;
}
CSymbol *MGenerate__FgenerateMethodCall(CNode *Am_node, CSContext *Actx, CSymbol *AdestSym) {
  CNode *Vnode;
  Vnode = Am_node->Vn_left;
  CNode *Varg_node;
  Varg_node = Am_node->Vn_right;
  if (MError__Vdebug)
  {
    (fputs(Zconcat("generateMethodCall() ", Vnode->Vn_string), stdout) | fputc('\n', stdout));
  }
  char *Vmethod;
  Vmethod = Vnode->Vn_string;
  CNode *Vvar_node;
  Vvar_node = Vnode->Vn_left;
  CSymbol *Vret;
  Vret = CSymbol__FNEW(0);
  Am_node->Vn_undefined = 0;
  CSymbol *Vsp;
  Vsp = NULL;
  if ((Vvar_node->Vn_type == 4))
  {
    Vsp = CSymbol__X__Vunknown;
    if (!(CScope__FisClassScope(Actx->Vscope)))
    {
      CNode__Ferror(Vnode, "PARENT not in class context");
    }
  else if ((Actx->Vscope->Vclass->VparentClass == NULL))
    {
      CNode__Ferror(Vnode, "PARENT not in a child class");
    }
  else
    {
      Vsp = CSymbol__FcopyObject(Actx->Vscope->Vclass->VparentClass);
    }
  }
else
  {
    Vsp = MGenerate__FgenerateVarname(Vvar_node, CSContext__FcopyNoOut(Actx), AdestSym);
  }
  if ((Vsp == NULL))
  {
    if ((Vvar_node->Vn_type == 7))
    {
      Vsp = CSymbol__FNEW(7);
    }
  else if ((Vvar_node->Vn_type == 9))
    {
      Vsp = CSymbol__FNEW(9);
    }
  }
  if ((Vsp == NULL))
  {
    Am_node->Vn_undefined = 3;
    if (MGenerate__FdoError(Actx->Vout))
    {
      MGenerate__FgenerateVarname(Vvar_node, Actx, AdestSym);
    }
    return Vret;
  }
  if ((Vsp->Vtype == 9))
  {
    if ((Zstrcmp(Vmethod, "slice") == 0))
    {
      CScope__FaddUsedItem(Actx->Vscope, "stringSlice");
      COutput__Fwrite(Actx->Vout, "ZStringSlice(");
      MGenerate__FgenerateVarname(Vvar_node, Actx, Vsp);
      Am_node->Vn_undefined += Vvar_node->Vn_undefined;
      COutput__Fwrite(Actx->Vout, ", ");
      MGenerate__FgenerateArguments(Varg_node, Actx, ZListAdd(ZListAdd(Zalloc(sizeof(CListHead)), -1, 0, CSymbol__X__Vint, 1), -1, 0, CSymbol__X__Vint, 1), 0);
      Am_node->Vn_undefined += Varg_node->Vn_undefined;
      COutput__Fwrite(Actx->Vout, ")");
      Vret->Vtype = 9;
    }
  else if ((Zstrcmp(Vmethod, "byteSlice") == 0))
    {
      CScope__FaddUsedItem(Actx->Vscope, "stringByteSlice");
      COutput__Fwrite(Actx->Vout, "ZStringByteSlice(");
      MGenerate__FgenerateVarname(Vvar_node, Actx, Vsp);
      Am_node->Vn_undefined += Vvar_node->Vn_undefined;
      COutput__Fwrite(Actx->Vout, ", ");
      MGenerate__FgenerateArguments(Varg_node, Actx, ZListAdd(ZListAdd(Zalloc(sizeof(CListHead)), -1, 0, CSymbol__X__Vint, 1), -1, 0, CSymbol__X__Vint, 1), 0);
      Am_node->Vn_undefined += Varg_node->Vn_undefined;
      COutput__Fwrite(Actx->Vout, ")");
      Vret->Vtype = 9;
    }
  else if ((Zstrcmp(Vmethod, "toInt") == 0))
    {
      CScope__FaddUsedItem(Actx->Vscope, "stringToInt");
      COutput__Fwrite(Actx->Vout, "ZStringToInt(");
      MGenerate__FgenerateVarname(Vvar_node, Actx, Vsp);
      Am_node->Vn_undefined += Vvar_node->Vn_undefined;
      COutput__Fwrite(Actx->Vout, ")");
      Vret->Vtype = 7;
    }
  else if ((Zstrcmp(Vmethod, "toLower") == 0))
    {
      CScope__FaddUsedItem(Actx->Vscope, "stringToLower");
      CScope__FaddUsedItem(Actx->Vscope, "alloc");
      COutput__Fwrite(Actx->Vout, "ZStringToLower(");
      MGenerate__FgenerateVarname(Vvar_node, Actx, Vsp);
      Am_node->Vn_undefined += Vvar_node->Vn_undefined;
      COutput__Fwrite(Actx->Vout, ")");
      Vret->Vtype = 9;
    }
  else if ((Zstrcmp(Vmethod, "toUpper") == 0))
    {
      CScope__FaddUsedItem(Actx->Vscope, "stringToUpper");
      CScope__FaddUsedItem(Actx->Vscope, "alloc");
      COutput__Fwrite(Actx->Vout, "ZStringToUpper(");
      MGenerate__FgenerateVarname(Vvar_node, Actx, Vsp);
      Am_node->Vn_undefined += Vvar_node->Vn_undefined;
      COutput__Fwrite(Actx->Vout, ")");
      Vret->Vtype = 9;
    }
  else if ((Zstrcmp(Vmethod, "binToInt") == 0))
    {
      CScope__FaddUsedItem(Actx->Vscope, "stringBinToInt");
      COutput__Fwrite(Actx->Vout, "ZStringBinToInt(");
      MGenerate__FgenerateVarname(Vvar_node, Actx, Vsp);
      Am_node->Vn_undefined += Vvar_node->Vn_undefined;
      COutput__Fwrite(Actx->Vout, ")");
      Vret->Vtype = 7;
    }
  else if ((Zstrcmp(Vmethod, "hexToInt") == 0))
    {
      CScope__FaddUsedItem(Actx->Vscope, "stringHexToInt");
      COutput__Fwrite(Actx->Vout, "ZStringHexToInt(");
      MGenerate__FgenerateVarname(Vvar_node, Actx, Vsp);
      Am_node->Vn_undefined += Vvar_node->Vn_undefined;
      COutput__Fwrite(Actx->Vout, ")");
      Vret->Vtype = 7;
    }
  else if ((Zstrcmp(Vmethod, "quotedToInt") == 0))
    {
      CScope__FaddUsedItem(Actx->Vscope, "stringQuotedToInt");
      COutput__Fwrite(Actx->Vout, "ZStringQuotedToInt(");
      MGenerate__FgenerateVarname(Vvar_node, Actx, Vsp);
      Am_node->Vn_undefined += Vvar_node->Vn_undefined;
      COutput__Fwrite(Actx->Vout, ")");
      Vret->Vtype = 7;
    }
  else if ((Zstrcmp(Vmethod, "quotedBinToInt") == 0))
    {
      CScope__FaddUsedItem(Actx->Vscope, "stringQuotedBinToInt");
      COutput__Fwrite(Actx->Vout, "ZStringQuotedBinToInt(");
      MGenerate__FgenerateVarname(Vvar_node, Actx, Vsp);
      Am_node->Vn_undefined += Vvar_node->Vn_undefined;
      COutput__Fwrite(Actx->Vout, ")");
      Vret->Vtype = 7;
    }
  else if ((Zstrcmp(Vmethod, "quotedHexToInt") == 0))
    {
      CScope__FaddUsedItem(Actx->Vscope, "stringQuotedHexToInt");
      COutput__Fwrite(Actx->Vout, "ZStringQuotedHexToInt(");
      MGenerate__FgenerateVarname(Vvar_node, Actx, Vsp);
      Am_node->Vn_undefined += Vvar_node->Vn_undefined;
      COutput__Fwrite(Actx->Vout, ")");
      Vret->Vtype = 7;
    }
  else if ((Zstrcmp(Vmethod, "SIZE") == 0))
    {
      COutput__Fwrite(Actx->Vout, "strlen(");
      MGenerate__FgenerateVarname(Vvar_node, Actx, Vsp);
      Am_node->Vn_undefined += Vvar_node->Vn_undefined;
      COutput__Fwrite(Actx->Vout, ")");
      Vret->Vtype = 7;
    }
  else if ((Zstrcmp(Vmethod, "index") == 0))
    {
      CScope__FaddUsedItem(Actx->Vscope, "strings.h");
      CScope__FaddUsedItem(Actx->Vscope, "stringIndex");
      COutput__Fwrite(Actx->Vout, "ZStringIndex(");
      MGenerate__FgenerateVarname(Vvar_node, Actx, Vsp);
      Am_node->Vn_undefined += Vvar_node->Vn_undefined;
      COutput__Fwrite(Actx->Vout, ", ");
      MGenerate__FgenExpr(Varg_node, Actx, CSymbol__X__Vint);
      Am_node->Vn_undefined += Varg_node->Vn_undefined;
      COutput__Fwrite(Actx->Vout, ")");
      Vret->Vtype = 7;
    }
  else if ((Zstrcmp(Vmethod, "find") == 0))
    {
      CScope__FaddUsedItem(Actx->Vscope, "strings.h");
      CScope__FaddUsedItem(Actx->Vscope, "stringFind");
      COutput__Fwrite(Actx->Vout, "ZStringFind(");
      MGenerate__FgenerateVarname(Vvar_node, Actx, Vsp);
      Am_node->Vn_undefined += Vvar_node->Vn_undefined;
      COutput__Fwrite(Actx->Vout, ", ");
      MGenerate__FgenExpr(Varg_node, Actx, CSymbol__X__Vstring);
      Am_node->Vn_undefined += Varg_node->Vn_undefined;
      COutput__Fwrite(Actx->Vout, ")");
      Vret->Vtype = 7;
    }
  else if ((Zstrcmp(Vmethod, "startsWith") == 0))
    {
      CScope__FaddUsedItem(Actx->Vscope, "strings.h");
      CScope__FaddUsedItem(Actx->Vscope, "stringStartsWith");
      COutput__Fwrite(Actx->Vout, "ZStringStartsWith(");
      MGenerate__FgenerateVarname(Vvar_node, Actx, Vsp);
      Am_node->Vn_undefined += Vvar_node->Vn_undefined;
      COutput__Fwrite(Actx->Vout, ", ");
      MGenerate__FgenExpr(Varg_node, Actx, CSymbol__X__Vstring);
      Am_node->Vn_undefined += Varg_node->Vn_undefined;
      COutput__Fwrite(Actx->Vout, ")");
      Vret->Vtype = 11;
    }
  else if ((Zstrcmp(Vmethod, "endsWith") == 0))
    {
      CScope__FaddUsedItem(Actx->Vscope, "strings.h");
      CScope__FaddUsedItem(Actx->Vscope, "stringEndsWith");
      COutput__Fwrite(Actx->Vout, "ZStringEndsWith(");
      MGenerate__FgenerateVarname(Vvar_node, Actx, Vsp);
      Am_node->Vn_undefined += Vvar_node->Vn_undefined;
      COutput__Fwrite(Actx->Vout, ", ");
      MGenerate__FgenExpr(Varg_node, Actx, CSymbol__X__Vstring);
      Am_node->Vn_undefined += Varg_node->Vn_undefined;
      COutput__Fwrite(Actx->Vout, ")");
      Vret->Vtype = 11;
    }
  else if ((Zstrcmp(Vmethod, "rindex") == 0))
    {
      CScope__FaddUsedItem(Actx->Vscope, "strings.h");
      CScope__FaddUsedItem(Actx->Vscope, "stringRindex");
      COutput__Fwrite(Actx->Vout, "ZStringRindex(");
      MGenerate__FgenerateVarname(Vvar_node, Actx, Vsp);
      Am_node->Vn_undefined += Vvar_node->Vn_undefined;
      COutput__Fwrite(Actx->Vout, ", ");
      MGenerate__FgenExpr(Varg_node, Actx, CSymbol__X__Vint);
      Am_node->Vn_undefined += Varg_node->Vn_undefined;
      COutput__Fwrite(Actx->Vout, ")");
      Vret->Vtype = 7;
    }
  else
    {
      ++(Am_node->Vn_undefined);
      if (MGenerate__FdoError(Actx->Vout))
      {
        MGenerate__Ferror(Zconcat(Zconcat("Method ", Vmethod), "() not supported for string"), Vnode);
      }
    }
  }
else if ((Vsp->Vtype == 7))
  {
    if ((Zstrcmp(Vmethod, "toChar") == 0))
    {
      CScope__FaddUsedItem(Actx->Vscope, "tochar");
      COutput__Fwrite(Actx->Vout, "Ztochar(");
      if ((Vvar_node->Vn_type == 7))
      {
        COutput__Fwrite(Actx->Vout, Zconcat(Zint2string(Vvar_node->Vn_int), ""));
      }
    else
      {
        MGenerate__FgenerateVarname(Vvar_node, Actx, Vsp);
        Am_node->Vn_undefined += Vvar_node->Vn_undefined;
      }
      COutput__Fwrite(Actx->Vout, ")");
      Vret->Vtype = 9;
    }
  else if ((Zstrcmp(Vmethod, "SIZE") == 0))
    {
      COutput__Fwrite(Actx->Vout, "sizeof(Zint)");
      Vret->Vtype = 7;
    }
  else
    {
      ++(Am_node->Vn_undefined);
      if (MGenerate__FdoError(Actx->Vout))
      {
        MGenerate__Ferror(Zconcat(Zconcat("Method ", Vmethod), "() not supported for int"), Vnode);
      }
    }
  }
else if ((((Vsp->Vtype == 29) || (Vsp->Vtype == 26)) || (Vsp->Vtype == 27)))
  {
    CSymbol *VenumSym;
    VenumSym = Vsp;
    Zbool VisBits;
    VisBits = 0;
    if ((Vsp->Vtype != 29))
    {
      VenumSym = Vsp->VreturnSymbol;
      VisBits = 1;
    }
    if (((Zstrcmp(Vmethod, "name") == 0) && (VenumSym != NULL)))
    {
      MGenerate__FgenEnumNameCall(VenumSym, Vnode, Actx);
      if (VisBits)
      {
        COutput__Fwrite(Actx->Vout, "(((");
      }
      MGenerate__FgenerateVarname(Vvar_node, Actx, VenumSym);
      Am_node->Vn_undefined += Vvar_node->Vn_undefined;
      if (VisBits)
      {
        COutput__Fwrite(Actx->Vout, Zconcat(Zconcat(") & ", (Zint2string((VenumSym->Vmask << VenumSym->Vvalue)))), ")"));
        if ((VenumSym->Vvalue > 0))
        {
          COutput__Fwrite(Actx->Vout, Zconcat(" >> ", Zint2string(VenumSym->Vvalue)));
        }
        COutput__Fwrite(Actx->Vout, ")");
      }
      COutput__Fwrite(Actx->Vout, ")");
      Vret->Vtype = 9;
    }
  else
    {
      ++(Am_node->Vn_undefined);
      if (MGenerate__FdoError(Actx->Vout))
      {
        MGenerate__Ferror(Zconcat(Zconcat("Method ", Vmethod), "() not supported for Enum"), Vnode);
      }
    }
  }
else if ((Vsp->Vtype == 13))
  {
    return MListStuff__FgenerateMethodCall(Vsp, Am_node, Actx, AdestSym);
  }
else if ((Vsp->Vtype == 14))
  {
    return MDictStuff__FgenerateMethodCall(Vsp, Am_node, Actx, AdestSym);
  }
else if (((Vsp->Vtype == 24) || (Vsp->Vtype == 25)))
  {
    return MGenerate__FgenerateObjectCall(Vsp, Am_node, Actx, AdestSym);
  }
else
  {
    ++(Am_node->Vn_undefined);
    if (MGenerate__FdoError(Actx->Vout))
    {
      if ((Vsp->Vtype == 21))
      {
        MGenerate__Ferror(Zconcat(Zconcat("Cannot invoke ", Vmethod), "() on a Class, expected an object or module"), Vnode);
      }
    else
      {
        MGenerate__Ferror(Zconcat(Zconcat(Zconcat("Method ", Vmethod), "() not supported for type "), Zenum2string(CNode__X__EType, sizeof(CNode__X__EType) / sizeof(char *), Vsp->Vtype)), Vnode);
      }
    }
  }
  return Vret;
}
void MGenerate__FgenEnumNameCall(CSymbol *AenumSym, CNode *Anode, CSContext *Actx) {
  if ((AenumSym->Vclass == NULL))
  {
    CNode__Ferror(Anode, "INTERNAL: Enum class is NIL");
  }
else
  {
    CScope__FaddUsedItem(Actx->Vscope, "enum2string");
    COutput__Fwrite(Actx->Vout, Zconcat(Zconcat(Zconcat(Zconcat("Zenum2string(", AenumSym->Vclass->VcName), ", sizeof("), AenumSym->Vclass->VcName), ") / sizeof(char *), "));
  }
}
CSymbol *MGenerate__FgenerateObjectCall(CSymbol *Asp, CNode *Am_node, CSContext *Actx, CSymbol *AdestSym) {
  CNode *Vnode;
  Vnode = Am_node->Vn_left;
  CNode *Varg_node;
  Varg_node = Am_node->Vn_right;
  CNode *Vvar_node;
  Vvar_node = Vnode->Vn_left;
  CSymbol *Vret;
  Vret = CSymbol__FNEW(0);
  CSymbol *Vclass;
  Vclass = Asp->Vclass;
  if ((Vclass == NULL))
  {
    Am_node->Vn_undefined = 2;
    if (MGenerate__FdoError(Actx->Vout))
    {
      MGenerate__Ferror("undefined class", Vnode);
    }
  }
else
  {
    CListHead *Varglist;
    Varglist = MGenerate__FgetSymbolListFromArgNode(Varg_node, Actx, 0);
    Zbool Vi_object_arg;
    Vi_object_arg = 0;
    char *VobjectArgName;
    VobjectArgName = "";
    if ((Varglist != NULL))
    {
      {
        Zfor_T *Zf = ZforNew(Varglist, 2);
        CSymbol *Vs;
        for (ZforGetPtr(Zf, (char **)&Vs); ZforCont(Zf); ZforNextPtr(Zf, (char **)&Vs)) {
          if ((Vs != NULL))
          {
            if ((Vs->Vtype == 25))
            {
              Vi_object_arg = 1;
              VobjectArgName = Zconcat(VobjectArgName, Zconcat(Zconcat("__", Vs->Vclass->VcName), "_I"));
            }
          else if (((Vs->Vtype == 24) || (Vs->Vtype == 21)))
            {
              VobjectArgName = Zconcat(VobjectArgName, Zconcat("__", Vs->Vclass->VcName));
            }
          else
            {
              VobjectArgName = Zconcat(VobjectArgName, Zconcat("__", Zenum2string(CNode__X__EType, sizeof(CNode__X__EType) / sizeof(char *), Vs->Vtype)));
            }
          }
        }
      }
    }
    CSymbol *Vp;
    Vp = MGenerate__FfindMethodArglist(Vclass, Vnode->Vn_string, Varglist, Actx->Vscope, 0, ((MGenerate__FdoError(Actx->Vout) && !(Vi_object_arg))) ? (Vnode) : (NULL), Zconcat("for class ", Vclass->Vname));
    if (((Vp == NULL) && Vi_object_arg))
    {
      Vp = CSymbol__FNEW(35);
      Vp->VmemberList = Varglist;
    }
    if ((Vp == NULL))
    {
      Am_node->Vn_undefined = 5;
    }
  else if ((Vp->Vtype == 111))
    {
      Am_node->Vn_undefined = 0;
      ((void (*)(CSymbol *, CSymbol *, CNode *, CSContext *))Vp->Vproduce)(Vp, Vclass, Am_node, Actx);
      Vret = Vp->VreturnSymbol;
    }
  else
    {
      Am_node->Vn_undefined = 0;
      if (((Vp->Vtype == 37) || (Vp->Vtype == 36)))
      {
        MGenerate__FgenerateRefCast(Vp, Vnode, Actx->Vout);
        MGenerate__FgenerateVarname(Vvar_node, Actx, Asp);
        Am_node->Vn_undefined += Vvar_node->Vn_undefined;
        COutput__Fwrite(Actx->Vout, Zconcat(Zconcat("->", Vp->VcName), ")("));
        Vret = Vp->VreturnSymbol;
      }
    else if (CSymbol__FisMethodType(Vp))
      {
        Zbool VvarNodeIsPointer;
        VvarNodeIsPointer = (Vvar_node->Vn_type != 5);
        if (((*(Zenum *)(Actx->Vgen->ptr + CResolve_I__VtargetLang_off[Actx->Vgen->type])) == 2))
        {
          if ((Vvar_node->Vn_type == 4))
          {
            COutput__Fwrite(Actx->Vout, Zconcat(Zconcat("this.parent__", Vp->Vname), "("));
          }
        else
          {
            MGenerate__FgenerateVarname(Vvar_node, Actx, Asp);
            COutput__Fwrite(Actx->Vout, Zconcat(Zconcat(".", Vp->Vname), "("));
          }
          Am_node->Vn_undefined += Vvar_node->Vn_undefined;
          Vret = Vp->VreturnSymbol;
        }
      else
        {
          if (((Asp->Vtype == 24) && !(Vi_object_arg)))
          {
            COutput__Fwrite(Actx->Vout, Zconcat(Vp->VcName, "("));
            MGenerate__FgenerateVarnameParent(Vvar_node, Actx, Asp);
            Am_node->Vn_undefined += Vvar_node->Vn_undefined;
            Vret = Vp->VreturnSymbol;
          }
        else if (((Asp->Vtype == 25) && !(Vi_object_arg)))
          {
            COutput__Fwrite(Actx->Vout, Zconcat(MGenerate__FclassFuncTableName(Vclass, Vp), "["));
            CSymbol *Vs;
            Vs = MGenerate__FgenerateVarnameParent(Vvar_node, Actx, Asp);
            COutput__Fwrite(Actx->Vout, "->type](");
            MGenerate__FgenerateVarnameParent(Vvar_node, Actx, Asp);
            Am_node->Vn_undefined += Vvar_node->Vn_undefined;
            COutput__Fwrite(Actx->Vout, "->ptr");
            Vret = Vp->VreturnSymbol;
          }
        else
          {
            char *VfuncName;
            VfuncName = Vclass->VcName;
            if ((Asp->Vtype == 25))
            {
              VfuncName = Zconcat(VfuncName, "_I");
            }
            VfuncName = Zconcat(VfuncName, Zconcat(Zconcat(Zconcat("__M", Vnode->Vn_string), "_I"), VobjectArgName));
            Vret = MGenerate__FgenerateVirtualFunc(VfuncName, Asp, Vnode, Varglist, Actx);
            Am_node->Vn_undefined += Vnode->Vn_undefined;
            COutput__Fwrite(Actx->Vout, Zconcat(VfuncName, "("));
            MGenerate__FgenerateVarnameParent(Vvar_node, Actx, Asp);
            Am_node->Vn_undefined += Vvar_node->Vn_undefined;
          }
          if (((Varg_node != NULL) && (Varg_node->Vn_type != 0)))
          {
            COutput__Fwrite(Actx->Vout, ", ");
          }
        }
      }
    else
      {
        if (MGenerate__FdoError(Actx->Vout))
        {
          MGenerate__Ferror(Zconcat(Zconcat("() after ", Zenum2string(CNode__X__EType, sizeof(CNode__X__EType) / sizeof(char *), Vp->Vtype)), " not expected"), Vnode);
        }
      }
      MGenerate__FgenerateArgumentsCheck(Varg_node, Vnode->Vn_string, Actx, CSymbol__FgetMemberList(Vp));
      Am_node->Vn_undefined += Varg_node->Vn_undefined;
      COutput__Fwrite(Actx->Vout, ")");
    }
  }
  return Vret;
}
CSymbol *MGenerate__FgenerateVirtualFunc(char *Aname, CSymbol *AvarSym, CNode *Anode, CListHead *Aarglist, CSContext *Actx) {
  CSymbol *Vdummy;
  Vdummy = CSymbol__FNEW(0);
  CSymbol *Vretsym;
  Vretsym = ZDictGetPtrDef(MGenerate__VvirtualFuncMap, 0, Aname, Vdummy);
  if ((Vretsym != Vdummy))
  {
    return Vretsym;
  }
  CNode *Vvar_node;
  Vvar_node = Anode->Vn_left;
  CListHead *ValtList;
  ValtList = Zalloc(sizeof(CListHead));
  CListHead *Valist;
  Valist = NULL;
  if ((AvarSym->Vtype == 25))
  {
    Valist = MGenerate__FinterfaceClassList(AvarSym->Vclass);
  }
else
  {
    Valist = Zalloc(sizeof(CListHead));
    if ((AvarSym->Vtype == 21))
    {
      ZListAdd(Valist, -1, 0, CSymbol__FcopyObject(AvarSym), 1);
    }
  else
    {
      ZListAdd(Valist, -1, 0, AvarSym, 1);
    }
  }
  ZListAdd(ValtList, -1, 0, Valist, 1);
  {
    Zfor_T *Zf = ZforNew(Aarglist, 2);
    CSymbol *Vs;
    for (ZforGetPtr(Zf, (char **)&Vs); ZforCont(Zf); ZforNextPtr(Zf, (char **)&Vs)) {
      if ((Vs != NULL))
      {
        if ((Vs->Vtype == 25))
        {
          Valist = MGenerate__FinterfaceClassList(Vs->Vclass);
        }
      else
        {
          Valist = Zalloc(sizeof(CListHead));
          if ((Vs->Vtype == 21))
          {
            ZListAdd(Valist, -1, 0, CSymbol__FcopyObject(Vs), 1);
          }
        else
          {
            ZListAdd(Valist, -1, 0, Vs, 1);
          }
        }
        ZListAdd(ValtList, -1, 0, Valist, 1);
      }
    }
  }
  COutput *VtmpOut;
  VtmpOut = COutput__FNEW(COutput__X__CFragmentHead__FNEW__1());
  VtmpOut->Vwriting = Actx->Vout->Vwriting;
  CListHead *Vindexes;
  Vindexes = Zalloc(sizeof(CListHead));
  {
    Zfor_T *Zf = ZforNew(ValtList, 2);
    CListHead *Vl;
    for (ZforGetPtr(Zf, (char **)&Vl); ZforCont(Zf); ZforNextPtr(Zf, (char **)&Vl)) {
      ZListAdd(Vindexes, -1, 0, NULL, 0);
    }
  }
  Zint Vdepth;
  Vdepth = 0;
  while (1)
  {
    Zint Vidx;
    Vidx = 0;
    CListHead *Vargtry;
    Vargtry = Zalloc(sizeof(CListHead));
    while ((Vidx < ValtList->itemCount))
    {
      if ((Vidx >= 1))
      {
        ZListAdd(Vargtry, -1, 0, ((CSymbol *)ZListGetPtr(ZListGetPtr(ValtList, Vidx), ZListGetInt(Vindexes, Vidx))), 1);
      }
      ++(Vidx);
    }
    CSymbol *Vclass;
    Vclass = ((CSymbol *)ZListGetPtr(ZListGetPtr(ValtList, 0), ZListGetInt(Vindexes, 0)));
    CSymbol *Vp;
    Vp = MGenerate__FfindMethodArglist(Vclass, Anode->Vn_string, Vargtry, Actx->Vscope, 1, (Actx->Vout->Vwriting) ? (Anode) : (NULL), "");
    if ((Vp == NULL))
    {
      ++(Anode->Vn_undefined);
    }
  else
    {
      if ((Vp->Vtype == 34))
      {
        Vretsym = Vp->VreturnSymbol;
      }
      while ((Vdepth < (2 * ValtList->itemCount)))
      {
        Vidx = (Vdepth / 2);
        CListHead *ValtListIdx;
        ValtListIdx = ZListGetPtr(ValtList, Vidx);
        if ((ValtListIdx->itemCount > 1))
        {
          if ((((Vdepth & 1)) == 0))
          {
            COutput__Fwrite(VtmpOut, Zconcat(Zconcat("switch (A", Zint2string(Vidx)), "->type) {\n"));
          }
        else
          {
            COutput__Fwrite(VtmpOut, Zconcat(Zconcat("case ", Zint2string(ZListGetInt(Vindexes, Vidx))), ":\n"));
          }
        }
        ++(Vdepth);
      }
      if ((Vp->Vtype == 34))
      {
        COutput__Fwrite(VtmpOut, "return ");
      }
      COutput__Fwrite(VtmpOut, Zconcat(Vp->VcName, "(A0"));
      if ((AvarSym->Vtype == 25))
      {
        COutput__Fwrite(VtmpOut, "->ptr");
      }
      Zint Vai;
      Vai = 0;
      while ((Vai < Vargtry->itemCount))
      {
        if (((((CSymbol *)ZListGetPtr(Vargtry, Vai))->Vtype != 25) && (((CSymbol *)ZListGetPtr(Vp->VmemberList, Vai))->Vtype != 25)))
        {
          COutput__Fwrite(VtmpOut, Zconcat(Zconcat(", (void *)(A", (Zint2string((Vai + 1)))), "->ptr)"));
        }
      else
        {
          COutput__Fwrite(VtmpOut, Zconcat(", A", (Zint2string((Vai + 1)))));
        }
        ++(Vai);
      }
      COutput__Fwrite(VtmpOut, ")");
      if ((Vp->Vtype != 34))
      {
        COutput__Fwrite(VtmpOut, "; return");
      }
      COutput__Fwrite(VtmpOut, ";\n");
    }
    Vidx = ValtList->itemCount;
    while ((Vidx > 0))
    {
      --(Vidx);
      if ((Vdepth > (Vidx * 2)))
      {
        --(Vdepth);
      }
      CListHead *ValtListIdx;
      ValtListIdx = ZListGetPtr(ValtList, Vidx);
      if ((ValtListIdx->itemCount > 1))
      {
        ++(*ZListGetIntP(Vindexes, Vidx));
        if ((ZListGetInt(Vindexes, Vidx) < ValtListIdx->itemCount))
        {
          break;
        }
        *ZListGetIntP(Vindexes, Vidx) = 0;
        if ((Vdepth > (Vidx * 2)))
        {
          --(Vdepth);
          COutput__Fwrite(VtmpOut, "}\n");
        }
      }
    else
      {
        --(Vdepth);
      }
    }
    if ((Vdepth == 0))
    {
      break;
    }
  }
  MGenerate__VvirtualOut->Vwriting = Actx->Vout->Vwriting;
  if ((Vretsym == NULL))
  {
    COutput__Fwrite(MGenerate__VvirtualOut, "void ");
  }
else
  {
    MGenerate__FgenType(Vretsym, Anode, MGenerate__VvirtualOut);
  }
  COutput__Fwrite(MGenerate__VvirtualOut, Zconcat(Aname, "("));
  if ((AvarSym->Vtype == 24))
  {
    MGenerate__FgenType(AvarSym->Vclass, Anode, MGenerate__VvirtualOut);
  }
else
  {
    MGenerate__FgenType(AvarSym, Anode, MGenerate__VvirtualOut);
  }
  COutput__Fwrite(MGenerate__VvirtualOut, "A0");
  Zint VargIdx;
  VargIdx = 1;
  {
    Zfor_T *Zf = ZforNew(Aarglist, 2);
    CSymbol *Vs;
    for (ZforGetPtr(Zf, (char **)&Vs); ZforCont(Zf); ZforNextPtr(Zf, (char **)&Vs)) {
      COutput__Fwrite(MGenerate__VvirtualOut, ", ");
      MGenerate__FgenType(Vs, Anode, MGenerate__VvirtualOut);
      COutput__Fwrite(MGenerate__VvirtualOut, Zconcat("A", Zint2string(VargIdx)));
      ++(VargIdx);
    }
  }
  COutput__Fwrite(MGenerate__VvirtualOut, ") {\n");
  COutput__Fappend(MGenerate__VvirtualOut, VtmpOut);
  COutput__Fwrite(MGenerate__VvirtualOut, "}\n");
  if (Actx->Vout->Vwriting)
  {
    ZDictAdd(1, MGenerate__VvirtualFuncMap, 0, Aname, 0, Vretsym);
  }
  return Vretsym;
}
CListHead *MGenerate__FinterfaceClassList(CSymbol *Asym) {
  CListHead *Volist;
  Volist = Zalloc(sizeof(CListHead));
  if (!((((Asym->Vattributes) & 1))))
  {
    if ((Asym->Vtype == 21))
    {
      ZListAdd(Volist, -1, 0, CSymbol__FcopyObject(Asym), 1);
    }
  else
    {
      ZListAdd(Volist, -1, 0, Asym, 1);
    }
  }
  if ((Asym->Vchildren != NULL))
  {
    {
      Zfor_T *Zf = ZforNew(Asym->Vchildren, 2);
      CSymbol *Vm;
      for (ZforGetPtr(Zf, (char **)&Vm); ZforCont(Zf); ZforNextPtr(Zf, (char **)&Vm)) {
        ZListExtend(Volist, MGenerate__FinterfaceClassList(Vm));
      }
    }
  }
  return Volist;
}
Zbool MGenerate__FclassOfInterface(CSymbol *Aitf, CSymbol *Achild) {
  if ((!((((Aitf->Vattributes) & 1))) && (Aitf == Achild)))
  {
    return 1;
  }
  if ((Aitf->Vchildren != NULL))
  {
    {
      Zfor_T *Zf = ZforNew(Aitf->Vchildren, 2);
      CSymbol *Vm;
      for (ZforGetPtr(Zf, (char **)&Vm); ZforCont(Zf); ZforNextPtr(Zf, (char **)&Vm)) {
        if (MGenerate__FclassOfInterface(Vm, Achild))
        {
          return 1;
        }
      }
    }
  }
  return 0;
}
CSymbol *MGenerate__FfindMethod(CSymbol *Aparent, char *Aname, CNode *Aargs, CSContext *Actx, Zbool AnoneOK, CNode *AmsgNode, char *Amsg) {
  CListHead *Varglist;
  Varglist = MGenerate__FgetSymbolListFromArgNode(Aargs, Actx, 0);
  return MGenerate__FfindMethodArglist(Aparent, Aname, Varglist, Actx->Vscope, AnoneOK, AmsgNode, Amsg);
}
CSymbol *MGenerate__FfindMethodArglist(CSymbol *Aparent, char *Aname, CListHead *Aarglist, CScope *Ascope, Zbool AnoneOK, CNode *AmsgNode, char *Amsg) {
  CSymbol *Vsym;
  Vsym = CSymbol__FfindMember(Aparent, Aname);
  if ((Vsym == NULL))
  {
    if ((!(AnoneOK) && (AmsgNode != NULL)))
    {
      CNode__Ferror(AmsgNode, Zconcat(Zconcat(Zconcat("Unknown method ", Aname), "() "), Amsg));
    }
  }
else if (((Vsym->Vtype != 37) && (Vsym->Vtype != 36)))
  {
    Zbool Vconvert;
    Vconvert = 0;
    Vsym = CSymbol__FfindMatchingFunction(Aparent, Aname, Aarglist, NULL, 1, 0);
    if ((Vsym == NULL))
    {
      Vconvert = 1;
      Vsym = CSymbol__FfindMatchingFunction(Aparent, Aname, Aarglist, NULL, 1, 1);
    }
    if ((AmsgNode != NULL))
    {
      if ((Vsym == NULL))
      {
        CNode__Ferror(AmsgNode, Zconcat(Zconcat(Zconcat("No method with matching arguments for ", Aname), "() "), Amsg));
        MGenerate__FlistMatchingMethods(Aname, Aparent, Aarglist);
      }
    else
      {
        if ((CSymbol__FfindMatchingFunction(Aparent, Aname, Aarglist, Vsym, 1, Vconvert) != NULL))
        {
          CNode__Ferror(AmsgNode, Zconcat(Zconcat(Zconcat("More than one method with matching arguments for ", Aname), "() "), Amsg));
          MGenerate__FlistMatchingMethods(Aname, Aparent, Aarglist);
          Vsym = NULL;
        }
      }
    }
  }
  return Vsym;
}
void MGenerate__FlistMatchingMethods(char *Aname, CSymbol *Aparent, CListHead *AargTypeList) {
  fputs(Zconcat("Expected: ", Aname), stdout);
  MGenerate__FlistArgTypes(AargTypeList);
  CSymbol *Vsym;
  Vsym = Aparent;
  while (1)
  {
    {
      Zfor_T *Zf = ZforNew(Vsym->VmemberList, 2);
      CSymbol *Vm;
      for (ZforGetPtr(Zf, (char **)&Vm); ZforCont(Zf); ZforNextPtr(Zf, (char **)&Vm)) {
        if ((Zstrcmp(Vm->Vname, Aname) == 0))
        {
          fputs(Zconcat("Candidate: ", Aname), stdout);
          MGenerate__FlistArgTypes(Vm->VmemberList);
        }
      }
    }
    if ((Vsym->VparentClass == NULL))
    {
      break;
    }
    Vsym = Vsym->VparentClass;
  }
}
void MGenerate__FlistArgTypes(CListHead *AargList) {
  fputs(MGenerate__FargTypesAsString(AargList), stdout);
}
char *MGenerate__FargTypesAsString(CListHead *AargList) {
  char *Vs;
  Vs = "(";
  if ((AargList != NULL))
  {
    char *Vcomma;
    Vcomma = "";
    {
      Zfor_T *Zf = ZforNew(AargList, 2);
      CSymbol *Va;
      for (ZforGetPtr(Zf, (char **)&Va); ZforCont(Zf); ZforNextPtr(Zf, (char **)&Va)) {
        Vs = Zconcat(Vs, Vcomma);
        if ((Va == NULL))
        {
          Vs = Zconcat(Vs, "NIL");
        }
      else if (((((Va->Vtype == 21) || (Va->Vtype == 24))) && (Va->Vclass != NULL)))
        {
          Vs = Zconcat(Vs, Va->Vclass->Vname);
        }
      else if (((Va->Vtype == 25) && (Va->Vclass != NULL)))
        {
          Vs = Zconcat(Vs, Zconcat(Va->Vclass->Vname, ".I"));
        }
      else
        {
          Vs = Zconcat(Vs, Zenum2string(CNode__X__EType, sizeof(CNode__X__EType) / sizeof(char *), Va->Vtype));
        }
        Vcomma = ", ";
      }
    }
  }
  Vs = Zconcat(Vs, ")\n");
  return Vs;
}
void MGenerate__FgenerateRefCast(CSymbol *Asym, CNode *Anode, COutput *Aout) {
  COutput__Fwrite(Aout, "((");
  if ((Asym->Vtype == 37))
  {
    COutput__Fwrite(Aout, "void");
  }
else if ((Asym->VreturnSymbol != NULL))
  {
    MGenerate__FgenType(Asym->VreturnSymbol, Anode, Aout);
  }
  COutput__Fwrite(Aout, " (*)(");
  char *Vcomma;
  Vcomma = "";
  if ((Asym->VmemberList != NULL))
  {
    {
      Zfor_T *Zf = ZforNew(Asym->VmemberList, 2);
      CSymbol *Varg;
      for (ZforGetPtr(Zf, (char **)&Varg); ZforCont(Zf); ZforNextPtr(Zf, (char **)&Varg)) {
        COutput__Fwrite(Aout, Vcomma);
        MGenerate__FgenType(Varg, Anode, Aout);
        Vcomma = ", ";
      }
    }
  }
  COutput__Fwrite(Aout, "))");
}
CListHead *MGenerate__FgetSymbolListFromArgNode(CNode *Anode, CSContext *Actx, Zbits Agsarg) {
  if (((Anode == NULL) || (Anode->Vn_type == 0)))
  {
    return NULL;
  }
  if ((Anode->Vn_type == 110))
  {
    CListHead *VsymLeft;
    VsymLeft = MGenerate__FgetSymbolListFromArgNode(Anode->Vn_left, Actx, Agsarg);
    CListHead *VsymRight;
    VsymRight = MGenerate__FgetSymbolListFromArgNode(Anode->Vn_right, Actx, Agsarg);
    Anode->Vn_undefined = (Anode->Vn_left->Vn_undefined + Anode->Vn_right->Vn_undefined);
    if ((VsymLeft == NULL))
    {
      return VsymRight;
    }
    if ((VsymRight == NULL))
    {
      return VsymLeft;
    }
    return ZListExtend(VsymLeft, VsymRight);
  }
  if (MError__Vdebug)
  {
    (fputs(Zconcat(Zconcat("getSymbolListFromArgNode() ", Zenum2string(CNode__X__EType, sizeof(CNode__X__EType) / sizeof(char *), Anode->Vn_type)), " node"), stdout) | fputc('\n', stdout));
  }
  CSymbol *Vsym;
  Vsym = NULL;
  if ((((Agsarg) & 1)))
  {
    Vsym = MGenerate__FgenerateObjDeclType(Anode, CSContext__FcopyNoOut(Actx));
  }
else
  {
    Vsym = MGenerate__FgenExpr(Anode, CSContext__FcopyNoOut(Actx), NULL);
  }
  if ((Vsym == NULL))
  {
    Vsym = CSymbol__FNEW(0);
    Anode->Vn_undefined = 10;
    if ((((Agsarg) & 2)))
    {
      CNode__Ferror(Anode, Zconcat("Unknown type: ", Anode->Vn_string));
    }
  }
else
  {
    if ((Vsym->Vtype == 0))
    {
      Anode->Vn_undefined = 2;
    }
  else
    {
      Anode->Vn_undefined = 0;
    }
    Vsym = CSymbol__Fcopy(Vsym);
  }
  return ZListAdd(Zalloc(sizeof(CListHead)), -1, 0, Vsym, 1);
}
CSymbol *MGenerate__FgenerateNewCall(CNode *Anode, CSContext *Actx, CSymbol *AdestSym) {
  CSymbol *VdestClass;
  VdestClass = AdestSym;
  Anode->Vn_undefined = 0;
  if (((VdestClass != NULL) && (VdestClass->Vtype == 24)))
  {
    VdestClass = CSymbol__FcopyObject(VdestClass);
  }
  CSymbol *Vsym;
  Vsym = AdestSym;
  if (((Anode->Vn_left != NULL) && (Anode->Vn_left->Vn_type == 109)))
  {
    Vsym = MGenerate__FgenerateDeclType(Anode->Vn_left, CSContext__FcopyNoOut(Actx));
    Anode->Vn_undefined += Anode->Vn_left->Vn_undefined;
  }
  if (((Vsym != NULL) && (Vsym->Vtype == 13)))
  {
    if ((((Anode->Vn_left != NULL) && (Anode->Vn_left->Vn_type != 13)) && (Anode->Vn_left->Vn_type != 109)))
    {
      CNode__Ferror(Anode, "Type.NEW() for list not supported");
    }
  else
    {
      CResolve_I__MwriteListAlloc__CSContext_ptr[Actx->Vgen->type](Actx->Vgen->ptr, Actx);
      return Vsym;
    }
  }
else if (((Vsym != NULL) && (Vsym->Vtype == 14)))
  {
    if ((((Anode->Vn_left != NULL) && (Anode->Vn_left->Vn_type != 14)) && (Anode->Vn_left->Vn_type != 109)))
    {
      CNode__Ferror(Anode, "Type.NEW() for dict not supported");
    }
  else if ((Vsym->VkeySymbol == NULL))
    {
      ++(Anode->Vn_undefined);
      if (MGenerate__FdoError(Actx->Vout))
      {
        CNode__Ferror(Anode, "Key type unknown");
      }
    }
  else
    {
      COutput__Fwrite(Actx->Vout, Zconcat(Zconcat("ZnewDict(", ((CSymbol__FisPointerType(Vsym->VkeySymbol)) ? ("1") : ("0"))), ")"));
      return Vsym;
    }
  }
else if (((Anode->Vn_left == NULL) && (((VdestClass == NULL) || (((VdestClass->Vtype != 21) && (VdestClass->Vtype != 24)))))))
  {
    ++(Anode->Vn_undefined);
    if (MGenerate__FdoError(Actx->Vout))
    {
      if ((VdestClass->Vtype == 25))
      {
        MGenerate__Ferror("Cannot use NEW() for an interface object", Anode);
      }
    else
      {
        MGenerate__Ferror("Do not know what class name to use for NEW()", Anode);
      }
    }
  }
else
  {
    CSymbol *Vclass;
    Vclass = NULL;
    if ((Anode->Vn_left == NULL))
    {
      Vclass = VdestClass;
      if ((Vclass->Vclass != NULL))
      {
        Vclass = Vclass->Vclass;
      }
    }
  else
    {
      Vclass = MGenerate__FgenerateDeclType(Anode->Vn_left, CSContext__FcopyNoOut(Actx));
      Anode->Vn_undefined += Anode->Vn_left->Vn_undefined;
      if (((Vclass == NULL) || (Vclass->Vtype != 21)))
      {
        ++(Anode->Vn_undefined);
        if (MGenerate__FdoError(Actx->Vout))
        {
          MGenerate__Ferror(Zconcat("class name not found: ", Anode->Vn_left->Vn_string), Anode);
        }
      }
    else if ((Vclass->Vclass != NULL))
      {
        Vclass = Vclass->Vclass;
      }
    }
    if ((((Vclass == NULL) || (Vclass->Vtype != 21)) || (((Vclass->Vattributes) & 1))))
    {
      ++(Anode->Vn_undefined);
      if (MGenerate__FdoError(Actx->Vout))
      {
        if ((Vclass != NULL))
        {
          MGenerate__Ferror(Zconcat("class not usable for NEW(): ", Vclass->Vname), Anode);
        }
      else
        {
          MGenerate__Ferror("class not usable for NEW()", Anode);
        }
      }
    }
  else
    {
      CSymbol *Vp;
      Vp = MGenerate__FfindMethod(Vclass, "NEW", Anode->Vn_right, Actx, 1, (MGenerate__FdoError(Actx->Vout)) ? (Anode) : (NULL), "");
      if ((Vp == NULL))
      {
        CResolve_I__MwriteAlloc__string__CSContext_ptr[Actx->Vgen->type](Actx->Vgen->ptr, Vclass->VcName, Actx);
      }
    else if ((Vp->Vproduce != NULL))
      {
        ((void (*)(CSymbol *, CSymbol *, CNode *, CSContext *))Vp->Vproduce)(Vp, Vclass, Anode, Actx);
      }
    else
      {
        COutput__Fwrite(Actx->Vout, Zconcat(Vp->VcName, "("));
        MGenerate__FgenerateArguments(Anode->Vn_right, Actx, CSymbol__FgetMemberList(Vp), 0);
        Anode->Vn_undefined += Anode->Vn_right->Vn_undefined;
        COutput__Fwrite(Actx->Vout, ")");
      }
      return CSymbol__FcopyObject(Vclass);
    }
  }
  return NULL;
}
void MGenerate__FlistMatchingFunc(CScope *Ascope, char *AfuncName, CListHead *AargTypeList, Zbool Aall) {
  fputs(Zconcat("Expected: ", AfuncName), stdout);
  MGenerate__FlistArgTypes(AargTypeList);
  CScope__FlistMatchingFunc(Ascope, AfuncName, AargTypeList, Aall);
}
CSymbol *MGenerate__FgenerateFunctionCall(CSymbol *Afunc, CNode *Anode, CSContext *Actx, Zenum Adest_type) {
  CListHead *Vargs;
  Vargs = CSymbol__FgetMemberList(Afunc);
  Zenum Vret_type;
  Vret_type = CSymbol__FgetReturnType(Afunc);
  CNode *Varg_node;
  Varg_node = Anode->Vn_right;
  CSymbol *Vret;
  Vret = Afunc->VreturnSymbol;
  if (((Adest_type != 0) && (((Vret == NULL) || (Vret->Vtype == 0)))))
  {
    if (MGenerate__FdoError(Actx->Vout))
    {
      CNode__Ferror(Anode, "PROC does not have a return value");
    }
  }
  CResolve_I__MwriteMethodCall__CSymbol__bool__CSContext_ptr[Actx->Vgen->type](Actx->Vgen->ptr, Afunc, (((Varg_node != NULL) && (Vargs != NULL)) && (Vargs->itemCount > 0)), Actx);
  MGenerate__FgenerateArguments(Varg_node, Actx, Vargs, 0);
  COutput__Fwrite(Actx->Vout, ")");
  Anode->Vn_undefined += Varg_node->Vn_undefined;
  return Vret;
}
void MGenerate__FgenerateArgumentsCheck(CNode *Anode, char *AfuncName, CSContext *Actx, CListHead *Aargs) {
  Zint Vn;
  Vn = MGenerate__FgenerateArguments(Anode, Actx, Aargs, 0);
  if (((Aargs != NULL) && (Vn < Aargs->itemCount)))
  {
    if (MGenerate__FdoError(Actx->Vout))
    {
      MGenerate__Ferror(Zconcat(Zconcat(Zconcat(Zconcat(Zconcat(Zconcat("Expected ", Zint2string(Aargs->itemCount)), " arguments for "), AfuncName), "()"), ", found only "), Zint2string(Vn)), Anode);
    }
  }
}
Zint MGenerate__FgenerateArguments(CNode *Anode, CSContext *Actx, CListHead *Aargs, Zint Aindex) {
  Zint Vnext_index;
  Vnext_index = Aindex;
  CSymbol *Vnext_arg;
  Vnext_arg = NULL;
  if ((Aargs != NULL))
  {
    Vnext_arg = ((CSymbol *)ZListGetPtr(Aargs, Aindex));
  }
  if (((Anode == NULL) || (Anode->Vn_type == 0)))
  {
    if ((Vnext_arg != NULL))
    {
      MGenerate__Ferror(Zconcat(Zconcat(Zconcat("missing ", Zenum2string(CNode__X__EType, sizeof(CNode__X__EType) / sizeof(char *), Vnext_arg->Vtype)), " argument "), (((Vnext_arg->Vname == NULL)) ? (Zint2string(Aindex)) : (Vnext_arg->Vname))), Anode);
    }
  }
else if ((Vnext_arg == NULL))
  {
    Anode->Vn_undefined = 1;
    if (MGenerate__FdoError(Actx->Vout))
    {
      MGenerate__Ferror(Zconcat("too many arguments, expected ", (((Aargs == NULL)) ? ("0") : (Zint2string(Aargs->itemCount)))), Anode);
    }
  }
else if ((Anode->Vn_type == 110))
  {
    Vnext_index = MGenerate__FgenerateArguments(Anode->Vn_left, Actx, Aargs, Aindex);
    COutput__Fwrite(Actx->Vout, ", ");
    Vnext_index = MGenerate__FgenerateArguments(Anode->Vn_right, Actx, Aargs, Vnext_index);
    Anode->Vn_undefined = (Anode->Vn_left->Vn_undefined + Anode->Vn_right->Vn_undefined);
  }
else
  {
    CSymbol *Vsym;
    Vsym = MGenerate__FgenExpr(Anode, CSContext__FcopyNoOut(Actx), Vnext_arg);
    char *Vclose;
    Vclose = "";
    if (((((Vsym != NULL) && (Vsym->Vtype == 24)) && (Vnext_arg->Vtype == 25)) && ((*(Zenum *)(Actx->Vgen->ptr + CResolve_I__VtargetLang_off[Actx->Vgen->type])) == 1)))
    {
      CScope__FaddUsedItem(Actx->Vscope, "allocZoref");
      COutput__Fwrite(Actx->Vout, "ZallocZoref(");
      Vclose = Zconcat(Zconcat(", ", Zint2string(CSymbol__FfindChild(Vnext_arg->Vclass, Vsym->Vclass))), ")");
    }
    MGenerate__FgenExpr(Anode, Actx, Vnext_arg);
    COutput__Fwrite(Actx->Vout, Vclose);
    ++(Vnext_index);
  }
  return Vnext_index;
}
CSymbol *MGenerate__FgenerateModuleCall(CSymbol *Asym, CNode *Aarg_node, CSContext *Actx) {
  Zenum Vfunc_type;
  Vfunc_type = Asym->Vtype;
  CSymbol *Vret;
  Vret = CSymbol__FNEW(0);
  if ((Vfunc_type == 111))
  {
    Aarg_node->Vn_undefined = 0;
    ((void (*)(CSymbol *, CSymbol *, CNode *, CSContext *))Asym->Vproduce)(Asym, NULL, Aarg_node, Actx);
    Vret = Asym->VreturnSymbol;
  }
else
  {
    Aarg_node->Vn_undefined = 10;
    if (MGenerate__FdoError(Actx->Vout))
    {
      MGenerate__Ferror(Zconcat("unexpected module function node type: ", Zenum2string(CNode__X__EType, sizeof(CNode__X__EType) / sizeof(char *), Vfunc_type)), Aarg_node);
    }
  }
  return Vret;
}
CSymbol *MGenerate__FgenerateModuleInfo(CSymbol *Asym, CNode *Anode, CSContext *Actx) {
  CSymbol *VinfoSym;
  VinfoSym = CScope__FgetSymbol(Actx->Vscope, "INFO");
  if ((VinfoSym == NULL))
  {
    Anode->Vn_undefined = 3;
    return NULL;
  }
  if ((Asym->Vscope == NULL))
  {
    CNode__Ferror(Anode, "INTERNAL: sym.scope not set");
    return NULL;
  }
  CSymbol *Vret;
  Vret = CSymbol__FfindMember(VinfoSym, "ModuleInfo");
  Asym->Vattributes = ((Asym->Vattributes) & -33) | ((1) << 5);
  if ((Vret == NULL))
  {
    Anode->Vn_undefined = 2;
    return NULL;
  }
  COutput__Fwrite(Actx->Vout, Zconcat(Asym->VcName, "__FINFO()"));
  return CSymbol__FcopyObject(Vret);
}
CSymbol *MGenerate__FgenerateVarnameParent(CNode *Anode, CSContext *Actx, CSymbol *AdestSym) {
  if ((Anode->Vn_type == 4))
  {
    COutput__Fwrite(Actx->Vout, "this");
    return AdestSym;
  }
  return MGenerate__FgenerateVarname(Anode, Actx, AdestSym);
}
CSymbol *MGenerate__FgenerateVarname(CNode *Anode, CSContext *Actx, CSymbol *AdestSym) {
  if ((Anode->Vn_type == 9))
  {
    MGenerate__FgenerateString(Anode, Actx);
    return CSymbol__X__Vstring;
  }
  return MGenerate__FgenerateLVarname(Anode, 0, Actx, AdestSym);
}
CSymbol *MGenerate__FgenerateLVarname(CNode *Anode, Zbool Alvalue, CSContext *Actx, CSymbol *AdestSym) {
  Anode->Vn_undefined = 0;
  CSymbol *Vsym;
  Vsym = NULL;
  if ((Anode->Vn_type == 105))
  {
    Vsym = CNode__FfindTopModule(Anode->Vn_left, Actx->Vscope);
  }
  if ((Vsym != NULL))
  {
    Vsym = CScope__FfindNodeSymbol(Actx->Vscope, Anode);
    if ((Vsym == NULL))
    {
      ++(Anode->Vn_undefined);
      if (MGenerate__FdoError(Actx->Vout))
      {
        MGenerate__Ferror(Zconcat(Zconcat("module member ", Anode->Vn_string), " not found"), Anode);
      }
    }
  else
    {
      COutput__Fwrite(Actx->Vout, Vsym->VcName);
    }
  }
else
  {
    Vsym = MGenerate__FgenerateVarnamePart(Anode, Alvalue, Actx, AdestSym);
  }
  return Vsym;
}
CSymbol *MGenerate__FgenerateVarnamePart(CNode *Anode, Zbool Alvalue, CSContext *Actx, CSymbol *AdestSym) {
  CSymbol *Vsp;
  Vsp = NULL;
  Zenum Vtype;
  Vtype = 0;
  if ((Anode->Vn_type == 5))
  {
    Vsp = CScope__FgetSymbol__2(Actx->Vscope, Anode->Vn_string, Anode);
    if ((Vsp == NULL))
    {
      ++(Anode->Vn_undefined);
      if (MGenerate__FdoError(Actx->Vout))
      {
        MGenerate__Ferror(Zconcat(Zconcat("symbol ", Anode->Vn_string), " not found"), Anode);
      }
    }
  else
    {
      CResolve_I__MwriteSymName__CSymbol__CSContext_ptr[Actx->Vgen->type](Actx->Vgen->ptr, Vsp, Actx);
      if ((Vsp->Vtype == 6))
      {
        Vsp = Vsp->VreturnSymbol;
        if ((Vsp == NULL))
        {
          ++(Anode->Vn_undefined);
          if (MGenerate__FdoError(Actx->Vout))
          {
            MGenerate__Ferror(Zconcat(Zconcat("symbol ", Anode->Vn_string), " type unknown"), Anode);
          }
        }
      }
    }
  }
else if ((Anode->Vn_type == 3))
  {
    if (!(CScope__FisClassScope(Actx->Vscope)))
    {
      CNode__Ferror(Anode, "THIS not in class context");
    }
  else
    {
      COutput__Fwrite(Actx->Vout, Actx->Vscope->VthisName);
      Vsp = CSymbol__FcopyObject(Actx->Vscope->Vclass);
    }
  }
else if ((Anode->Vn_type == 4))
  {
    CNode__Ferror(Anode, "invalid use of PARENT");
  }
else if ((Anode->Vn_type == 107))
  {
    Vsp = MGenerate__FgenerateCall(Anode, Actx, AdestSym, 0);
  }
else if ((Anode->Vn_type == 59))
  {
    Vsp = MGenerate__FgenerateNewCall(Anode, Actx, AdestSym);
  }
else if ((Anode->Vn_type == 105))
  {
    Anode->Vn_left->Vn_undefined = 0;
    Vsp = MGenerate__FgenerateVarnamePart(Anode->Vn_left, 0, CSContext__FcopyNoOut(Actx), AdestSym);
    Anode->Vn_undefined += Anode->Vn_left->Vn_undefined;
    if ((Vsp == NULL))
    {
      if (MGenerate__FdoError(Actx->Vout))
      {
        MGenerate__Ferror(Zconcat("Error before .", Anode->Vn_string), Anode);
      }
    }
  else if ((Vsp->Vtype == 25))
    {
      CSymbol *Vmember;
      Vmember = CSymbol__FfindMember(Vsp->Vclass, Anode->Vn_string);
      if ((Vmember == NULL))
      {
        ++(Anode->Vn_undefined);
        if (MGenerate__FdoError(Actx->Vout))
        {
          CNode__Ferror(Anode, Zconcat(Zconcat("member \"", Anode->Vn_string), "\" not found"));
        }
      }
    else if (((*(Zenum *)(Actx->Vgen->ptr + CResolve_I__VtargetLang_off[Actx->Vgen->type])) == 1))
      {
        COutput__Fwrite(Actx->Vout, "(*(");
        MGenerate__FgenType(Vmember, Anode, Actx->Vout);
        COutput__Fwrite(Actx->Vout, "*)(");
        MGenerate__FgenerateVarnamePart(Anode->Vn_left, 0, Actx, AdestSym);
        COutput__Fwrite(Actx->Vout, Zconcat(Zconcat("->ptr + ", MGenerate__FclassOffTableName(Vsp->Vclass, Vmember)), "["));
        Anode->Vn_left->Vn_undefined = 0;
        MGenerate__FgenerateVarnamePart(Anode->Vn_left, 0, Actx, AdestSym);
        COutput__Fwrite(Actx->Vout, "->type]))");
        Vsp = Vmember;
        Anode->Vn_undefined += Anode->Vn_left->Vn_undefined;
      }
    else
      {
        MGenerate__FgenerateVarnamePart(Anode->Vn_left, 0, Actx, AdestSym);
        Anode->Vn_undefined += Anode->Vn_left->Vn_undefined;
        COutput__Fwrite(Actx->Vout, Zconcat(".", Vmember->VcName));
        Vsp = Vmember;
      }
    }
  else if ((Vsp->Vtype == 21))
    {
      CSymbol *Vm;
      Vm = CSymbol__FfindMember(Vsp, Anode->Vn_string);
      if ((Vm == NULL))
      {
        Anode->Vn_undefined += 2;
        if (MGenerate__FdoError(Actx->Vout))
        {
          MGenerate__Ferror(Zconcat(Zconcat("no member ", Anode->Vn_string), " in this class"), Anode);
        }
      }
    else if ((Vm->VcName[0] != 67))
      {
        ++(Anode->Vn_undefined);
        if (MGenerate__FdoError(Actx->Vout))
        {
          MGenerate__Ferror(Zconcat(Zconcat("member ", Anode->Vn_string), " in not in SHARED section of class"), Anode);
        }
      }
    else
      {
        COutput__Fwrite(Actx->Vout, Vm->VcName);
        Vsp = Vm;
      }
    }
  else
    {
      Anode->Vn_left->Vn_undefined = 0;
      Vsp = MGenerate__FgenerateVarnamePart(Anode->Vn_left, 0, Actx, AdestSym);
      Anode->Vn_undefined += Anode->Vn_left->Vn_undefined;
      if ((Vsp->Vtype == 24))
      {
        Vsp = CSymbol__FfindMember(Vsp, Anode->Vn_string);
        if ((Vsp == NULL))
        {
          ++(Anode->Vn_undefined);
          if (MGenerate__FdoError(Actx->Vout))
          {
            MGenerate__Ferror(Zconcat(Zconcat("member \"", Anode->Vn_string), "\" not found"), Anode);
          }
        }
      else
        {
          CResolve_I__Mmember__string__COutput_ptr[Actx->Vgen->type](Actx->Vgen->ptr, Vsp->VcName, Actx->Vout);
        }
      }
    else if (((Vsp->Vtype == 26) || (Vsp->Vtype == 27)))
      {
        CSymbol *Vm;
        Vm = CSymbol__FfindMember(Vsp, Anode->Vn_string);
        if ((Vm == NULL))
        {
          ++(Anode->Vn_undefined);
          if (MGenerate__FdoError(Actx->Vout))
          {
            MGenerate__Ferror(Zconcat(Zconcat("member ", Anode->Vn_string), " not found"), Anode);
          }
        }
      else
        {
          Vsp = CSymbol__Fcopy(Vsp);
          Vsp->VreturnSymbol = Vm;
        }
      }
    else
      {
        ++(Anode->Vn_undefined);
        if (MGenerate__FdoError(Actx->Vout))
        {
          MGenerate__Ferror(Zconcat(Zconcat(Zconcat("Unexpected type ", Zenum2string(CNode__X__EType, sizeof(CNode__X__EType) / sizeof(char *), Vsp->Vtype)), "  before ."), Anode->Vn_string), Anode);
        }
      }
    }
  }
else if ((Anode->Vn_type == 108))
  {
    Vsp = MGenerate__FgenerateVarnamePart(Anode->Vn_left, 0, CSContext__FcopyNoOut(Actx), AdestSym);
    if (((Vsp != NULL) && (Vsp->Vtype == 13)))
    {
      MListStuff__FgenerateSubscript(Vsp, Anode, Alvalue, Actx, AdestSym);
      if ((Vsp->VreturnSymbol != NULL))
      {
        Vsp = CSymbol__Fcopy(Vsp->VreturnSymbol);
      }
    }
  else if (((Vsp != NULL) && (Vsp->Vtype == 14)))
    {
      MDictStuff__FgenerateSubscript(Vsp, Anode, Alvalue, Actx, AdestSym);
      if ((Vsp->VreturnSymbol != NULL))
      {
        Vsp = CSymbol__Fcopy(Vsp->VreturnSymbol);
      }
    }
  else
    {
      Anode->Vn_left->Vn_undefined = 0;
      Vsp = MGenerate__FgenerateVarnamePart(Anode->Vn_left, 0, Actx, AdestSym);
      Anode->Vn_undefined += Anode->Vn_left->Vn_undefined;
      COutput__Fwrite(Actx->Vout, "[");
      MGenerate__FgenExpr(Anode->Vn_right, Actx, CSymbol__X__Vint);
      Anode->Vn_undefined += Anode->Vn_right->Vn_undefined;
      COutput__Fwrite(Actx->Vout, "]");
      if (((Vsp != NULL) && (Vsp->Vtype == 9)))
      {
        Vsp = CSymbol__FNEW(7);
      }
    else if (((Vsp != NULL) && (Vsp->Vtype == 16)))
      {
        Vsp = CSymbol__Fcopy(Vsp->VreturnSymbol);
      }
    }
  }
else
  {
    ++(Anode->Vn_undefined);
    if (MGenerate__FdoError(Actx->Vout))
    {
      Zenum Vt;
      Vt = Anode->Vn_type;
      MGenerate__Ferror(Zconcat(Zconcat("node type ", Zenum2string(CNode__X__EType, sizeof(CNode__X__EType) / sizeof(char *), Vt)), " not expected"), Anode);
    }
  }
  return Vsp;
}
Zbool MGenerate__FcompatibleTypes(Zenum Asrc_type, Zenum Adest_type) {
  return (((Adest_type == Asrc_type) || (Adest_type == 0)) || (((Adest_type == 9) && ((((Asrc_type == 7) || (Asrc_type == 11)) || (Asrc_type == 12))))));
}
Zbool MGenerate__FcompatibleSymbols(CSymbol *Asrc, CSymbol *Adest) {
  if ((!(MGenerate__FcompatibleTypes(Asrc->Vtype, Adest->Vtype)) && (((((Asrc->Vtype != 24) && (Asrc->Vtype != 21))) || ((((Adest->Vtype != 24) && (Adest->Vtype != 25)) && (Adest->Vtype != 21)))))))
  {
    return 0;
  }
  if (((((Asrc->Vtype == 21) || (Asrc->Vtype == 24)) || (Asrc->Vtype == 25))))
  {
    if (((Asrc->Vclass == NULL) || (Adest->Vclass == NULL)))
    {
      return 0;
    }
    if ((Asrc->Vclass == Adest->Vclass))
    {
      return 1;
    }
    if (((Adest->Vtype == 25) && MGenerate__FclassOfInterface(Adest->Vclass, ((Asrc->Vtype == 24)) ? (Asrc->Vclass) : (Asrc))))
    {
      return 1;
    }
    return 0;
  }
  return 1;
}
CSymbol *MGenerate__FgenExpr(CNode *Anode, CSContext *Actx, CSymbol *AdestSym) {
  Zint VerrorCount;
  VerrorCount = MError__VerrorCount;
  CSymbol *Vret;
  Vret = CResolve_I__Mexpr__CNode__CSContext__CSymbol_ptr[Actx->Vgen->type](Actx->Vgen->ptr, Anode, Actx, AdestSym);
  if (((((AdestSym != NULL) && (Vret != NULL)) && Actx->Vout->Vwriting) && (MError__VerrorCount == VerrorCount)))
  {
    Zenum Vdest_type;
    Vdest_type = AdestSym->Vtype;
    if ((Vret->Vtype != Vdest_type))
    {
      MGenerate__FtypeError(Vdest_type, Vret->Vtype, Anode);
    }
  else if (((((Vdest_type == 24) && (AdestSym->Vclass != NULL)) && (Vret->Vclass != NULL)) && (Zstrcmp(AdestSym->Vclass->Vname, Vret->Vclass->Vname) != 0)))
    {
      MGenerate__Ferror(Zconcat(Zconcat(Zconcat("Expected ", AdestSym->Vclass->Vname), " but found "), Vret->Vclass->Vname), Anode);
    }
  else if (((((Vdest_type == 13) && (AdestSym->VreturnSymbol != NULL)) && (Vret->VreturnSymbol != NULL)) && (AdestSym->VreturnSymbol->Vtype != Vret->VreturnSymbol->Vtype)))
    {
      MGenerate__FtypeError(AdestSym->VreturnSymbol->Vtype, Vret->VreturnSymbol->Vtype, Anode);
    }
  else if (((((Vdest_type == 14) && (AdestSym->VkeySymbol != NULL)) && (Vret->VkeySymbol != NULL)) && (AdestSym->VkeySymbol->Vtype != Vret->VkeySymbol->Vtype)))
    {
      MGenerate__FtypeError(AdestSym->VkeySymbol->Vtype, Vret->VkeySymbol->Vtype, Anode);
    }
  else if (((((Vdest_type == 14) && (AdestSym->VreturnSymbol != NULL)) && (Vret->VreturnSymbol != NULL)) && (AdestSym->VreturnSymbol->Vtype != Vret->VreturnSymbol->Vtype)))
    {
      MGenerate__FtypeError(AdestSym->VreturnSymbol->Vtype, Vret->VreturnSymbol->Vtype, Anode);
    }
  }
  return Vret;
}
CSymbol *MGenerate__FgenExprChecked(CNode *Anode, CSContext *Actx, CSymbol *AdestSym) {
  COutput *Vout;
  Vout = Actx->Vout;
  CScope *Vscope;
  Vscope = Actx->Vscope;
  Zenum Vdest_type;
  Vdest_type = (((AdestSym == NULL))) ? (0) : (AdestSym->Vtype);
  if (MError__Vdebug)
  {
    if ((Vscope->VimportIndent != NULL))
    {
      fputs(Vscope->VimportIndent, stdout);
    }
    fputs("genExpr() ", stdout);
    fputs(Zconcat(Zenum2string(CNode__X__EType, sizeof(CNode__X__EType) / sizeof(char *), Anode->Vn_type), " node"), stdout);
    if ((Anode->Vn_string != NULL))
    {
      fputs(Zconcat(Zconcat(" \"", Anode->Vn_string), "\""), stdout);
    }
    if ((AdestSym != NULL))
    {
      fputs(Zconcat(Zconcat("; dest type: ", Zenum2string(CNode__X__EType, sizeof(CNode__X__EType) / sizeof(char *), AdestSym->Vtype)), " "), stdout);
    }
    fputs(Zconcat("writing: ", Zbool2string(Vout->Vwriting)), stdout);
    fputs("\n", stdout);
  }
  if (((AdestSym != NULL) && (((Vdest_type == 26) || (Vdest_type == 27)))))
  {
    CSymbol *Vret;
    Vret = CSymbol__Fcopy(AdestSym);
    switch (Anode->Vn_type)
    {
    case 5:
      {
        {
          CSymbol *VmemberSym;
          VmemberSym = CSymbol__FfindMember(AdestSym, Anode->Vn_string);
          if ((VmemberSym == NULL))
          {
            break;
          }
          Anode->Vn_undefined = 0;
          if ((VmemberSym->Vtype != 11))
          {
            Anode->Vn_undefined = 1;
            if (MGenerate__FdoError(Vout))
            {
              MGenerate__FtypeError(11, VmemberSym->Vtype, Anode);
            }
          }
          Vret->Vvalue = (1 << VmemberSym->Vvalue);
          COutput__Fwrite(Vout, Zconcat("", Zint2string(Vret->Vvalue)));
          return Vret;
        }
          break;
      }
    case 7:
      {
        {
          if ((Anode->Vn_int == 0))
          {
            Vret->Vvalue = 0;
            COutput__Fwrite(Vout, "0");
            Anode->Vn_undefined = 0;
            return Vret;
          }
        }
          break;
      }
    case 80:
      {
        {
          COutput__Fwrite(Vout, "(");
          MGenerate__FgenExpr(Anode->Vn_left, Actx, AdestSym);
          COutput__Fwrite(Vout, " + ");
          MGenerate__FgenExpr(Anode->Vn_right, Actx, AdestSym);
          COutput__Fwrite(Vout, ")");
          Anode->Vn_undefined = (Anode->Vn_left->Vn_undefined + Anode->Vn_right->Vn_undefined);
          return Vret;
        }
          break;
      }
    case 106:
      {
        {
          if ((Anode->Vn_left->Vn_type != 5))
          {
            break;
          }
          CSymbol *VmemberSym;
          VmemberSym = CSymbol__FfindMember(AdestSym, Anode->Vn_left->Vn_string);
          if ((VmemberSym == NULL))
          {
            break;
          }
          Anode->Vn_undefined = 0;
          if ((VmemberSym->Vtype == 7))
          {
            if (((Anode->Vn_right->Vn_int > (VmemberSym->Vmask >> 1)) || (Anode->Vn_right->Vn_int < (-(VmemberSym->Vmask) >> 1))))
            {
              Anode->Vn_undefined = 1;
              CNode__Ferror(Anode, Zconcat("value out of range: ", Zint2string(Anode->Vn_right->Vn_int)));
            }
            Vret->Vvalue = (Anode->Vn_right->Vn_int << VmemberSym->Vvalue);
            COutput__Fwrite(Vout, Zconcat("", Zint2string(Vret->Vvalue)));
            return Vret;
          }
          if ((VmemberSym->Vtype == 8))
          {
            if (((Anode->Vn_right->Vn_int & ~(VmemberSym->Vmask)) != 0))
            {
              Anode->Vn_undefined = 1;
              CNode__Ferror(Anode, Zconcat("value out of range: ", Zint2string(Anode->Vn_right->Vn_int)));
            }
            Vret->Vvalue = (Anode->Vn_right->Vn_int << VmemberSym->Vvalue);
            COutput__Fwrite(Vout, Zconcat("", Zint2string(Vret->Vvalue)));
            return Vret;
          }
          if ((VmemberSym->Vtype != 29))
          {
            Anode->Vn_undefined = 2;
            if (MGenerate__FdoError(Vout))
            {
              MGenerate__FtypeError(29, VmemberSym->Vtype, Anode);
            }
          }
        else
          {
            CSymbol *VvalueSym;
            VvalueSym = CSymbol__FfindMember(VmemberSym, Anode->Vn_right->Vn_string);
            if ((VvalueSym == NULL))
            {
              Anode->Vn_undefined = 1;
              CNode__Ferror(Anode, Zconcat("Unknown enum value: ", Anode->Vn_right->Vn_string));
            }
          else
            {
              Vret->Vvalue = (VvalueSym->Vvalue << VmemberSym->Vvalue);
              COutput__Fwrite(Vout, Zconcat("", Zint2string(Vret->Vvalue)));
            }
          }
          return Vret;
        }
          break;
      }
    }
  }
  if ((Vdest_type == 21))
  {
    Vdest_type = 24;
  }
  CSymbol *Vret;
  Vret = CSymbol__FNEW(Vdest_type);
  Vret->Vvalue = 1;
  switch (Anode->Vn_type)
  {
  case 11:
    {
      {
        if ((Vdest_type == 9))
        {
          if ((Anode->Vn_int == 0))
          {
            COutput__Fwrite(Vout, "\"FALSE\"");
          }
        else
          {
            COutput__Fwrite(Vout, "\"TRUE\"");
          }
          Vret->Vtype = 9;
        }
      else
        {
          if ((Anode->Vn_int == 0))
          {
            COutput__Fwrite(Vout, "0");
          }
        else
          {
            COutput__Fwrite(Vout, "1");
          }
          Vret->Vtype = 11;
        }
      }
        break;
    }
  case 12:
    {
      {
        if ((Vdest_type == 9))
        {
          if ((Anode->Vn_int == 0))
          {
            COutput__Fwrite(Vout, "\"FAIL\"");
          }
        else
          {
            COutput__Fwrite(Vout, "\"OK\"");
          }
          Vret->Vtype = 9;
        }
      else
        {
          if ((Anode->Vn_int == 0))
          {
            COutput__Fwrite(Vout, "0");
          }
        else
          {
            COutput__Fwrite(Vout, "1");
          }
          Vret->Vtype = 12;
        }
      }
        break;
    }
  case 7:
    {
      {
        if ((Vdest_type == 9))
        {
          COutput__Fwrite(Vout, Zconcat(Zconcat("\"", Zint2string(Anode->Vn_int)), "\""));
        }
      else
        {
          Vret->Vtype = 7;
          Vret->Vvalue = Anode->Vn_int;
          COutput__Fwrite(Vout, Zconcat(Zint2string(Anode->Vn_int), ""));
        }
      }
        break;
    }
  case 9:
    {
      {
        MGenerate__FgenerateString(Anode, Actx);
        Vret->Vtype = 9;
      }
        break;
    }
  case 2:
    {
      {
        Anode->Vn_undefined = 0;
        if (!(CNode__X__FisPointerType(Vdest_type)))
        {
          Anode->Vn_undefined = 1;
          if (MGenerate__FdoError(Vout))
          {
            MGenerate__FtypeError(Vdest_type, 2, Anode);
          }
        }
        CResolve_I__Mnil__CSContext_ptr[Actx->Vgen->type](Actx->Vgen->ptr, Actx);
      }
        break;
    }
  case 3:
    {
      {
        Anode->Vn_undefined = 0;
        if ((((Vdest_type != 24) && (Vdest_type != 25)) && (Vdest_type != 2)))
        {
          Anode->Vn_undefined = 1;
          if (MGenerate__FdoError(Vout))
          {
            MGenerate__FtypeError(Vdest_type, 24, Anode);
          }
        }
      else if (!(CScope__FisClassScope(Vscope)))
        {
          Anode->Vn_undefined = 2;
          CNode__Ferror(Anode, "THIS cannot be used here");
        }
        COutput__Fwrite(Vout, Actx->Vscope->VthisName);
        Vret->Vtype = 24;
        Vret->Vclass = Vscope->Vclass;
      }
        break;
    }
  case 4:
    {
      {
        CNode__Ferror(Anode, "PARENT cannot be used here");
      }
        break;
    }
  case 82:
    {
      {
        COutput__Fwrite(Vout, "-(");
        MGenerate__FgenExpr(Anode->Vn_left, Actx, CSymbol__X__Vint);
        COutput__Fwrite(Vout, ")");
        Vret->Vtype = 7;
        Anode->Vn_undefined = Anode->Vn_left->Vn_undefined;
      }
        break;
    }
  case 84:
    {
      {
        COutput__Fwrite(Vout, "!(");
        MGenerate__FgenExpr(Anode->Vn_left, Actx, CSymbol__X__Vbool);
        COutput__Fwrite(Vout, ")");
        Vret->Vtype = 11;
        Anode->Vn_undefined = Anode->Vn_left->Vn_undefined;
      }
        break;
    }
  case 83:
    {
      {
        COutput__Fwrite(Vout, "~(");
        MGenerate__FgenExpr(Anode->Vn_left, Actx, CSymbol__X__Vint);
        COutput__Fwrite(Vout, ")");
        Vret->Vtype = 7;
        Anode->Vn_undefined = Anode->Vn_left->Vn_undefined;
      }
        break;
    }
  case 6:
    {
      {
        Anode->Vn_undefined = 0;
        if (((AdestSym == NULL) || (AdestSym->Vtype != 6)))
        {
          Anode->Vn_undefined = 5;
          if (MGenerate__FdoError(Vout))
          {
            CNode__Ferror(Anode, "Unexpected &");
          }
        }
      else
        {
          COutput__Fwrite(Vout, "&(");
          Vret->VreturnSymbol = MGenerate__FgenerateVarname(Anode->Vn_left, Actx, AdestSym->VreturnSymbol);
          Anode->Vn_undefined += Anode->Vn_left->Vn_undefined;
          Vret->Vtype = 6;
          COutput__Fwrite(Vout, ")");
        }
      }
        break;
    }
  case 5:
    {
      {
        Vret = CResolve_I__Mid__CNode__CSContext__CSymbol_ptr[Actx->Vgen->type](Actx->Vgen->ptr, Anode, Actx, AdestSym);
      }
        break;
    }
  case 107:
    {
      {
        Vret = MGenerate__FgenerateCall(Anode, Actx, AdestSym, 1);
      }
        break;
    }
  case 59:
    {
      {
        Vret = MGenerate__FgenerateNewCall(Anode, Actx, AdestSym);
      }
        break;
    }
  case 105:
    {
      {
        CSymbol *VnodeSym;
        VnodeSym = CScope__FfindNodeSymbol(Vscope, Anode->Vn_left);
        if (((VnodeSym != NULL) && (VnodeSym->Vtype == 29)))
        {
          MGenerate__FgenEnumMember(Anode, VnodeSym, AdestSym, Vout, Vret);
        }
      else if (((VnodeSym != NULL) && (VnodeSym->Vtype == 21)))
        {
          MGenerate__FgenModuleMember(Anode, Vdest_type, Actx, Vret);
        }
      else if (((VnodeSym != NULL) && (((VnodeSym->Vtype == 26) || (VnodeSym->Vtype == 27)))))
        {
          MGenerate__FgenBitsMember(Anode, VnodeSym, Vdest_type, Actx, Vret);
        }
      else
        {
          CSymbol *VmoduleSym;
          VmoduleSym = CNode__FfindTopModule(Anode->Vn_left, Vscope);
          if ((VmoduleSym != NULL))
          {
            Vret = MGenerate__FgenModuleMember(Anode, Vdest_type, Actx, Vret);
          }
        else
          {
            Vret = MGenerate__FgenClassMember(Anode, Vdest_type, Actx, Vret);
          }
        }
      }
        break;
    }
  case 108:
    {
      {
        Vret = CResolve_I__Msubscript__CNode__CSContext__CSymbol_ptr[Actx->Vgen->type](Actx->Vgen->ptr, Anode, Actx, AdestSym);
      }
        break;
    }
  case 73:
  case 74:
  case 75:
  case 77:
  case 78:
  case 79:
  case 89:
  case 90:
  case 81:
    {
      {
        CResolve_I__MnumberOp__CNode__CSContext_ptr[Actx->Vgen->type](Actx->Vgen->ptr, Anode, Actx);
        Vret->Vtype = 7;
      }
        break;
    }
  case 76:
    {
      {
        CResolve_I__MconcatOp__CNode__CSContext_ptr[Actx->Vgen->type](Actx->Vgen->ptr, Anode, Actx);
        Vret->Vtype = 9;
      }
        break;
    }
  case 80:
    {
      {
        Vret->Vtype = CResolve_I__MplusOp__CNode__CSContext__CNode__X__EType_ptr[Actx->Vgen->type](Actx->Vgen->ptr, Anode, Actx, Vdest_type);
      }
        break;
    }
  case 85:
  case 86:
  case 87:
  case 88:
    {
      {
        CResolve_I__MincrdecrOp__CNode__CSContext_ptr[Actx->Vgen->type](Actx->Vgen->ptr, Anode, Actx);
        Vret->Vtype = 7;
      }
        break;
    }
  case 91:
  case 92:
  case 97:
  case 98:
    {
      {
        CResolve_I__MbooleanOp__CNode__CSContext_ptr[Actx->Vgen->type](Actx->Vgen->ptr, Anode, Actx);
        Vret->Vtype = 11;
      }
        break;
    }
  case 99:
  case 100:
    {
      {
        MGenerate__Ferror("ISA and ISNOTA not implemented yet", Anode);
      }
        break;
    }
  case 93:
  case 94:
  case 95:
  case 96:
    {
      {
        CResolve_I__McompareOp__CNode__CSContext_ptr[Actx->Vgen->type](Actx->Vgen->ptr, Anode, Actx);
        Vret->Vtype = 11;
      }
        break;
    }
  case 101:
  case 102:
    {
      {
        CResolve_I__MandorOp__CNode__CSContext_ptr[Actx->Vgen->type](Actx->Vgen->ptr, Anode, Actx);
        Vret->Vtype = 11;
      }
        break;
    }
  case 104:
    {
      {
        Vret = CResolve_I__Mparens__CNode__CSContext__CSymbol_ptr[Actx->Vgen->type](Actx->Vgen->ptr, Anode, Actx, AdestSym);
      }
        break;
    }
  case 103:
    {
      {
        Vret = CResolve_I__MaltOp__CNode__CSContext__CSymbol_ptr[Actx->Vgen->type](Actx->Vgen->ptr, Anode, Actx, AdestSym);
      }
        break;
    }
  case 13:
    {
      {
        Vret->Vtype = 13;
        Vret->VreturnSymbol = CResolve_I__MlistPart__CNode__CSContext__CSymbol_ptr[Actx->Vgen->type](Actx->Vgen->ptr, Anode->Vn_right, Actx, ((AdestSym == NULL)) ? (NULL) : (AdestSym->VreturnSymbol));
        Anode->Vn_undefined = Anode->Vn_right->Vn_undefined;
        if (((Vret->VreturnSymbol != NULL) && (Vret->VreturnSymbol->Vtype == 21)))
        {
          Vret->VreturnSymbol = CSymbol__FcopyObject(Vret->VreturnSymbol);
        }
      }
        break;
    }
  case 14:
    {
      {
        Vret->Vtype = 14;
        CResolve_I__MdictPart__CNode__CSymbol__CSContext_ptr[Actx->Vgen->type](Actx->Vgen->ptr, Anode->Vn_right, Vret, Actx);
        Anode->Vn_undefined = Anode->Vn_right->Vn_undefined;
      }
        break;
    }
  default:
    {
      {
        if ((Anode->Vn_type == 0))
        {
          MGenerate__Ferror("Missing argument", Anode);
        }
      else if (MGenerate__FdoError(Vout))
        {
          MGenerate__Ferror(Zconcat("INTERNAL: Node type not supported: ", Zenum2string(CNode__X__EType, sizeof(CNode__X__EType) / sizeof(char *), Anode->Vn_type)), Anode);
        }
      }
        break;
    }
  }
  return Vret;
}
void MGenerate__FgenerateString(CNode *Anode, CSContext *Actx) {
  COutput *Vout;
  Vout = Actx->Vout;
  COutput__Fwrite(Vout, "\"");
  Zint Vi;
  Vi = 0;
  while ((Vi < strlen(Anode->Vn_string)))
  {
    Zint Vc;
    Vc = Anode->Vn_string[Vi];
    if ((Vc == 10))
    {
      COutput__Fwrite(Vout, "\\n");
    }
  else if ((Vc == 13))
    {
      COutput__Fwrite(Vout, "\\r");
    }
  else if ((Vc == 9))
    {
      COutput__Fwrite(Vout, "\\t");
    }
  else if ((Vc == 34))
    {
      COutput__Fwrite(Vout, "\\\"");
    }
  else if ((Vc == 92))
    {
      COutput__Fwrite(Vout, "\\\\");
    }
  else
    {
      COutput__Fwrite(Vout, Ztochar(Vc));
    }
    ++(Vi);
  }
  COutput__Fwrite(Vout, "\"");
}
void MGenerate__FgenEnumMember(CNode *Anode, CSymbol *AnodeSym, CSymbol *AdestSym, COutput *Aout, CSymbol *Aret) {
  Zint Vvalue;
  Vvalue = -(1);
  CListHead *VmemberList;
  VmemberList = CSymbol__FgetMemberList(AnodeSym);
  if ((VmemberList == NULL))
  {
    Anode->Vn_undefined = 3;
    MGenerate__Ferror("Enum without members", Anode);
  }
else
  {
    CSymbol *Vmember_sym;
    Vmember_sym = CSymbol__X__Ffind(VmemberList, Anode->Vn_string);
    if ((Vmember_sym == NULL))
    {
      Anode->Vn_undefined = 2;
      MGenerate__Ferror(Zconcat("No such enum value: ", Anode->Vn_string), Anode);
    }
  else
    {
      Vvalue = Vmember_sym->Vvalue;
      Zenum Vdest_type;
      Vdest_type = (((AdestSym == NULL))) ? (0) : (AdestSym->Vtype);
      Anode->Vn_undefined = 0;
      if ((Vdest_type != 0))
      {
        if ((Vdest_type != 29))
        {
          Anode->Vn_undefined = 1;
          if (MGenerate__FdoError(Aout))
          {
            MGenerate__FtypeError(Vdest_type, 29, Anode);
          }
        }
      else if (!(((AdestSym->Vclass == AnodeSym))))
        {
          Anode->Vn_undefined = 1;
          if (MGenerate__FdoError(Aout))
          {
            MGenerate__Ferror(Zconcat(Zconcat(Zconcat("Expected ENUM ", AdestSym->Vclass->Vname), " but found ENUM "), AnodeSym->Vname), Anode);
          }
        }
      }
      Aret->Vtype = 29;
      Aret->Vclass = AnodeSym;
    }
  }
  COutput__Fwrite(Aout, Zconcat(Zint2string(Vvalue), ""));
}
void MGenerate__FgenBitsMember(CNode *Anode, CSymbol *AnodeSym, Zenum Adest_type, CSContext *Actx, CSymbol *Aret) {
  CListHead *VmemberList;
  VmemberList = CSymbol__FgetMemberList(AnodeSym);
  if ((VmemberList == NULL))
  {
    MGenerate__Ferror("BITS without members", Anode);
    Anode->Vn_undefined = 2;
  }
else
  {
    CSymbol *Vmember_sym;
    Vmember_sym = CSymbol__X__Ffind(VmemberList, Anode->Vn_string);
    if ((Vmember_sym == NULL))
    {
      Anode->Vn_undefined = 1;
      if (MGenerate__FdoError(Actx->Vout))
      {
        CNode__Ferror(Anode, Zconcat("No such member: ", Anode->Vn_string));
      }
    }
  else
    {
      if ((Vmember_sym->Vtype == 11))
      {
        if ((((Adest_type != 0) && (Adest_type != 9)) && (Adest_type != 11)))
        {
          ++(Anode->Vn_undefined);
          if (MGenerate__FdoError(Actx->Vout))
          {
            MGenerate__FtypeError(Adest_type, 11, Anode);
          }
        }
        Aret->Vtype = 11;
        COutput__Fwrite(Actx->Vout, "(((");
        MGenerate__FgenExpr(Anode->Vn_left, Actx, NULL);
        Anode->Vn_undefined = Anode->Vn_left->Vn_undefined;
        Zint Vshift;
        Vshift = Vmember_sym->Vvalue;
        COutput__Fwrite(Actx->Vout, Zconcat(Zconcat(") & ", (Zint2string((1 << Vshift)))), ")"));
        COutput__Fwrite(Actx->Vout, ")");
      }
    else if (((Vmember_sym->Vtype == 7) || (Vmember_sym->Vtype == 8)))
      {
        if ((((Adest_type != 0) && (Adest_type != 9)) && (Adest_type != 7)))
        {
          ++(Anode->Vn_undefined);
          if (MGenerate__FdoError(Actx->Vout))
          {
            MGenerate__FtypeError(Adest_type, 7, Anode);
          }
        }
        Aret->Vtype = 7;
        if ((Vmember_sym->Vtype == 7))
        {
          CScope__FaddUsedItem(Actx->Vscope, "fixSign");
          COutput__Fwrite(Actx->Vout, "ZFixSign(");
        }
        COutput__Fwrite(Actx->Vout, "(((");
        MGenerate__FgenExpr(Anode->Vn_left, Actx, NULL);
        Anode->Vn_undefined = Anode->Vn_left->Vn_undefined;
        Zint Vshift;
        Vshift = Vmember_sym->Vvalue;
        COutput__Fwrite(Actx->Vout, Zconcat(Zconcat(") & ", (Zint2string((Vmember_sym->Vmask << Vshift)))), ")"));
        if ((Vshift > 0))
        {
          COutput__Fwrite(Actx->Vout, Zconcat(" >> ", Zint2string(Vshift)));
        }
        COutput__Fwrite(Actx->Vout, ")");
        if ((Vmember_sym->Vtype == 7))
        {
          COutput__Fwrite(Actx->Vout, Zconcat(Zconcat(", ", Zint2string(~(((Vmember_sym->Vmask >> 1))))), ")"));
        }
      }
    else if ((Vmember_sym->Vtype == 29))
      {
        if (((Adest_type != 0) && (Adest_type != 9)))
        {
          if ((Adest_type != 29))
          {
            ++(Anode->Vn_undefined);
            if (MGenerate__FdoError(Actx->Vout))
            {
              MGenerate__FtypeError(Adest_type, 11, Anode);
            }
          }
        }
        if ((Adest_type == 9))
        {
          MGenerate__FgenEnumNameCall(Vmember_sym, Anode, Actx);
        }
      else
        {
          Aret->Vtype = 29;
        }
        COutput__Fwrite(Actx->Vout, "(((");
        MGenerate__FgenExpr(Anode->Vn_left, Actx, NULL);
        Anode->Vn_undefined = Anode->Vn_left->Vn_undefined;
        Zint Vshift;
        Vshift = Vmember_sym->Vvalue;
        COutput__Fwrite(Actx->Vout, Zconcat(Zconcat(") & ", (Zint2string((Vmember_sym->Vmask << Vshift)))), ")"));
        if ((Vshift > 0))
        {
          COutput__Fwrite(Actx->Vout, Zconcat(" >> ", Zint2string(Vshift)));
        }
        COutput__Fwrite(Actx->Vout, ")");
        if ((Adest_type == 9))
        {
          COutput__Fwrite(Actx->Vout, ")");
        }
      }
    else
      {
        CNode__Ferror(Anode, "INTERNAL: Not implemented yet");
      }
    }
  }
}
CSymbol *MGenerate__FgenModuleMember(CNode *Anode, Zenum Adest_type, CSContext *Actx, CSymbol *Aret_in) {
  CSymbol *Vret;
  Vret = Aret_in;
  CSymbol *VmoduleSym;
  VmoduleSym = CScope__FfindNodeSymbol__1(Actx->Vscope, Anode->Vn_left, 0, MGenerate__FdoError(Actx->Vout));
  if ((VmoduleSym == NULL))
  {
    Anode->Vn_undefined = 3;
  }
else
  {
    CListHead *VmemberList;
    VmemberList = CSymbol__FgetMemberList(VmoduleSym);
    if ((VmemberList == NULL))
    {
      Anode->Vn_undefined = 2;
      MGenerate__Ferror("Module without members", Anode);
    }
  else
    {
      CSymbol *Vsym;
      Vsym = CSymbol__X__Ffind(VmemberList, Anode->Vn_string);
      if ((Vsym == NULL))
      {
        Anode->Vn_undefined = 1;
        MGenerate__Ferror(Zconcat("No such module member: ", Anode->Vn_string), Anode);
      }
    else
      {
        Anode->Vn_undefined = 0;
        if (((Vsym->Vtype == 7) && (Adest_type == 9)))
        {
        }
      else if ((Vsym->Vtype != Adest_type))
        {
          Anode->Vn_undefined = 1;
          if (MGenerate__FdoError(Actx->Vout))
          {
            MGenerate__FtypeError(Adest_type, Vsym->Vtype, Anode);
          }
        }
        COutput__Fwrite(Actx->Vout, Vsym->VcName);
        Vret = CSymbol__Fcopy(Vsym);
      }
    }
  }
  return Vret;
}
CSymbol *MGenerate__FgenClassMember(CNode *Anode, Zenum Adest_type, CSContext *Actx, CSymbol *Aret_in) {
  CSymbol *Vret;
  Vret = Aret_in;
  CSymbol *VleftSym;
  VleftSym = NULL;
  CSymbol *VdestSym;
  VdestSym = NULL;
  CSymbol *VparentSym;
  VparentSym = NULL;
  VleftSym = MGenerate__FgenExpr(Anode->Vn_left, CSContext__FcopyNoOut(Actx), CSymbol__X__Vparent);
  Anode->Vn_undefined = Anode->Vn_left->Vn_undefined;
  Zenum Vmember_type;
  Vmember_type = 0;
  CSymbol *Vsym;
  Vsym = NULL;
  Zenum Vtype;
  Vtype = 0;
  if ((VleftSym == NULL))
  {
    Vret = NULL;
    if (MGenerate__FdoError(Actx->Vout))
    {
      VleftSym = MGenerate__FgenExpr(Anode->Vn_left, Actx, CSymbol__X__Vparent);
    }
  }
else
  {
    VparentSym = VleftSym;
    VdestSym = VleftSym;
    if ((VleftSym->Vtype == 6))
    {
      if ((VleftSym->VreturnSymbol != NULL))
      {
        VparentSym = VleftSym->VreturnSymbol;
        VdestSym = VparentSym;
      }
    else
      {
        ++(Anode->Vn_undefined);
      }
    }
    if ((VleftSym->Vtype == 25))
    {
      VparentSym = VleftSym->Vclass;
      Vtype = 24;
    }
  else
    {
      Vtype = VdestSym->Vtype;
    }
    Vsym = CSymbol__FfindMember(VparentSym, Anode->Vn_string);
    if ((Vsym == NULL))
    {
      Vmember_type = 0;
    }
  else
    {
      Vmember_type = Vsym->Vtype;
    }
  }
  if (((Vtype == 24) && (Vsym != NULL)))
  {
    if (MGenerate__FdoError(Actx->Vout))
    {
      MGenerate__FcheckExprType(Adest_type, Vmember_type, Anode);
    }
    if ((VleftSym->Vtype == 25))
    {
      CResolve_I__MiobjectMember__CSymbol__CSymbol__CNode__CSContext__CSymbol_ptr[Actx->Vgen->type](Actx->Vgen->ptr, Vsym, VleftSym, Anode, Actx, VdestSym);
    }
  else
    {
      MGenerate__FgenExpr(Anode->Vn_left, Actx, VdestSym);
      CResolve_I__Mmember__string__COutput_ptr[Actx->Vgen->type](Actx->Vgen->ptr, Vsym->VcName, Actx->Vout);
      Anode->Vn_undefined = Anode->Vn_left->Vn_undefined;
    }
    if ((Vsym->Vtype == 0))
    {
      ++(Anode->Vn_undefined);
    }
    Vret->Vtype = Vmember_type;
    Vret->Vclass = Vsym->Vclass;
    Vret->VcName = Vsym->VcName;
    Vret->VclassName = Vsym->VclassName;
    Vret->VmemberList = Vsym->VmemberList;
    Vret->VreturnSymbol = Vsym->VreturnSymbol;
    Vret->VparentClass = Vsym->VparentClass;
    Vret->Vattributes = Vsym->Vattributes;
  }
else
  {
    Anode->Vn_undefined = 10;
    if (MGenerate__FdoError(Actx->Vout))
    {
      if (((Anode->Vn_left == NULL) || (Anode->Vn_left->Vn_type != 5)))
      {
        MGenerate__Ferror(Zconcat("unknown member: ", Anode->Vn_string), Anode);
      }
    else
      {
        MGenerate__Ferror(Zconcat(Zconcat(Zconcat("unknown member: ", Anode->Vn_left->Vn_string), "."), Anode->Vn_string), Anode);
      }
    }
  }
  return Vret;
}
void MGenerate__Ferror(char *Amsg, CNode *Anode) {
  if ((Anode == NULL))
  {
    MError__Freport(Amsg);
  }
else
  {
    CNode__Ferror(Anode, Amsg);
  }
  fflush(stdout);
}
void MGenerate__FtypeError(Zenum Aexpected, Zenum Aactual, CNode *Anode) {
  if (((Aexpected != 0) && !((((Aexpected == 2) && CNode__X__FisPointerType(Aactual))))))
  {
    if (CNode__X__FisTypeWithArgs(Aactual))
    {
      MGenerate__Ferror("Missing ()", Anode);
    }
  else
    {
      MGenerate__Ferror(Zconcat(Zconcat(Zconcat("Type mismatch: expected ", Zenum2string(CNode__X__EType, sizeof(CNode__X__EType) / sizeof(char *), Aexpected)), " but found "), Zenum2string(CNode__X__EType, sizeof(CNode__X__EType) / sizeof(char *), Aactual)), Anode);
    }
  }
}
void MGenerate__FcheckExprType(Zenum Aexpected, Zenum Aactual, CNode *Anode) {
  if (!(MGenerate__FcompatibleTypes(Aactual, Aexpected)))
  {
    MGenerate__FtypeError(Aexpected, Aactual, Anode);
  }
}
void MGenerate__FgenType(CSymbol *Asym, CNode *Anode, COutput *Aout) {
  switch (Asym->Vtype)
  {
  case 21:
  case 24:
    {
      {
        if ((((Asym->Vattributes) & 1)))
        {
          MGenerate__Ferror(Zconcat("Cannot use abstract class ", Asym->Vname), Anode);
        }
        if ((Asym->Vclass != NULL))
        {
          COutput__Fwrite(Aout, Zconcat(Asym->Vclass->VclassName, " *"));
        }
      else
        {
          COutput__Fwrite(Aout, Zconcat(Asym->VclassName, " *"));
        }
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
        MGenerate__FgenType(Asym->VreturnSymbol, Anode, Aout);
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
        if (MGenerate__FdoError(Aout))
        {
          MGenerate__Ferror(Zconcat("Declaration of unknown type ", Zenum2string(CNode__X__EType, sizeof(CNode__X__EType) / sizeof(char *), Asym->Vtype)), Anode);
        }
      }
        break;
    }
  }
}
void Igenerate() {
 static int done = 0;
 if (!done) {
  done = 1;
  Ibuiltin();
  Iconfig();
  Idictstuff();
  Iexpreval();
  Iinput();
  Iliststuff();
  Ioutput();
  Iparse();
  Iresolve();
  Iscontext();
  Iscope();
  Isymbol();
  Iusedfile();
  Iwrite_c();
  Iwrite_js();
  Izimbufile();
  Izwtvalues();
  MGenerate__VscopeNumber = 1;
  MGenerate__VshowErrors = 0;
  MGenerate__VnoOut = COutput__FNEW(NULL);
  MGenerate__Vskip_zero_undefined = 1;
  MGenerate__VimportedFiles = Zalloc(sizeof(CListHead));
  MGenerate__VvirtualFuncMap = ZnewDict(1);
  MGenerate__VvirtualOut = COutput__FNEW(COutput__X__CFragmentHead__FNEW__1());
 }
}
