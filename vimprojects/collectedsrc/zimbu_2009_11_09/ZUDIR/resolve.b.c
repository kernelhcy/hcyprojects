#define INC_resolve_B 1
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
#ifndef INC_write_c_B
#include "../ZUDIR/write_c.b.c"
#endif
#ifndef INC_write_js_B
#include "../ZUDIR/write_js.b.c"
#endif
CResolve *CResolve__FNEW() {
  CResolve *THIS = Zalloc(sizeof(CResolve));
  THIS->VtargetLang = 0;
  return THIS;
}
char *CResolve__FgetLangName(CResolve *THIS) {
  return "none";
}
char *CResolve__FthisName(CResolve *THIS, Zbool AinsideNew) {
  return "this";
}
CZimbuFile__X__CCodeSpecific *CResolve__FgetCS(CResolve *THIS, CZimbuFile *AzimbuFile) {
  MError__Freport("INTERNAL: Resolve.getCS() should not be called");
  return NULL;
}
void CResolve__FmainHead(CResolve *THIS, CSContext *Actx) {
}
void CResolve__FmainEnd(CResolve *THIS, CNode *AmainNode, CSContext *Actx) {
}
void CResolve__FcopyC(CResolve *THIS, CNode *Anode, CSContext *Actx) {
}
void CResolve__FcopyJS(CResolve *THIS, CNode *Anode, CSContext *Actx) {
}
void CResolve__FwriteAlloc(CResolve *THIS, char *AtypeName, CSContext *Actx) {
}
void CResolve__FwriteListAlloc(CResolve *THIS, CSContext *Actx) {
}
void CResolve__FwriteNewThis(CResolve *THIS, CSymbol *Asym, CSContext *Actx) {
}
void CResolve__FwriteNewReturn(CResolve *THIS, COutput *Aout) {
}
void CResolve__FwriteSymName(CResolve *THIS, CSymbol *Asym, CSContext *Actx) {
}
CSymbol *CResolve__Fid(CResolve *THIS, CNode *Anode, CSContext *Actx, CSymbol *AdestSym) {
  CSymbol *Vret;
  Vret = NULL;
  Zenum Vdest_type;
  Vdest_type = ((AdestSym == NULL)) ? (0) : (AdestSym->Vtype);
  CSymbol *Vsym;
  Vsym = CScope__FgetSymbol__2(Actx->Vscope, Anode->Vn_string, Anode);
  if ((Vsym == NULL))
  {
    Anode->Vn_undefined = 2;
  }
else
  {
    if ((Vsym->Vtype == 0))
    {
      Anode->Vn_undefined = 1;
    }
  else
    {
      Anode->Vn_undefined = 0;
    }
    CSymbol *VunrefSym;
    VunrefSym = Vsym;
    if ((Vsym->Vtype == 6))
    {
      if ((Vsym->VreturnSymbol == NULL))
      {
        ++(Anode->Vn_undefined);
      }
    else
      {
        if ((Vdest_type == 6))
        {
          Vsym = Vsym->VreturnSymbol;
        }
      else
        {
          VunrefSym = Vsym->VreturnSymbol;
        }
      }
    }
    Vret = CSymbol__Fcopy(VunrefSym);
    Zenum Vtype;
    Vtype = VunrefSym->Vtype;
    if ((((MGenerate__FcompatibleTypes(Vret->Vtype, Vdest_type) || (((AdestSym != NULL) && MGenerate__FcompatibleSymbols(Vret, AdestSym)))) || (((Vdest_type == 4) && (((((Vtype == 19) || (Vtype == 24)) || (Vtype == 21)) || (Vtype == 29)))))) || (((Vdest_type == 2) && CNode__X__FisPointerType(Vtype)))))
    {
    }
  else if (((Vdest_type == 37) && (Vtype == 35)))
    {
      Vret->Vtype = Vdest_type;
    }
  else if (((Vdest_type == 36) && (Vtype == 34)))
    {
      Vret->Vtype = Vdest_type;
    }
  else
    {
      ++(Anode->Vn_undefined);
    }
    if ((Vsym->Vtype != 0))
    {
      Anode->Vn_symbol = CSymbol__Fcopy(Vsym);
      Anode->Vn_returnSymbol = Vret;
    }
  }
  return Vret;
}
CSymbol *CResolve__FmethodReturnType(CResolve *THIS, CNode *Anode, CSymbol *Asym, CSContext *Actx) {
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
    VretSym = MGenerate__FgenerateDeclType(Anode->Vn_returnType, Actx);
  }
  return VretSym;
}
void CResolve__FwriteMethodCall(CResolve *THIS, CSymbol *Afunc, Zbool AmoreArgs, CSContext *Actx) {
}
void CResolve__FargWithType(CResolve *THIS, Zbool Afirst, CSymbol *AtypeSym, CNode *AtypeNode, char *AargName, CSContext *Actx) {
}
Zbool CResolve__FdoWriteDecl(CResolve *THIS) {
  return 0;
}
CSymbol *CResolve__Fsubscript(CResolve *THIS, CNode *Anode, CSContext *Actx, CSymbol *AdestSym) {
  CSymbol *Vret;
  Vret = NULL;
  CSymbol *Vsym;
  Vsym = MGenerate__FgenExpr(Anode->Vn_left, Actx, NULL);
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
      MGenerate__FgenExpr(Anode->Vn_right, Actx, CSymbol__X__Vint);
      Anode->Vn_undefined = (Anode->Vn_left->Vn_undefined + Anode->Vn_right->Vn_undefined);
      if ((Vsym->VreturnSymbol != NULL))
      {
        Vret = CSymbol__Fcopy(Vsym->VreturnSymbol);
      }
    else
      {
        ++(Anode->Vn_undefined);
      }
    }
  else if ((Vsym->Vtype == 9))
    {
      MGenerate__FgenExpr(Anode->Vn_left, Actx, CSymbol__X__Vstring);
      MGenerate__FgenExpr(Anode->Vn_right, Actx, CSymbol__X__Vint);
      Anode->Vn_undefined = (Anode->Vn_left->Vn_undefined + Anode->Vn_right->Vn_undefined);
      Vret = CSymbol__X__Vint;
    }
  else
    {
      Anode->Vn_undefined = 7;
    }
  }
else
  {
    Anode->Vn_undefined = 10;
  }
  if ((AdestSym != NULL))
  {
    Anode->Vn_symbol = Vsym;
  }
  return Vret;
}
void CResolve__FiobjectMember(CResolve *THIS, CSymbol *AobjSym, CSymbol *AitfSym, CNode *Anode, CSContext *Actx, CSymbol *AdestSym) {
}
void CResolve__FnumberOp(CResolve *THIS, CNode *Anode, CSContext *Actx) {
  MGenerate__FgenExpr(Anode->Vn_left, Actx, CSymbol__X__Vint);
  MGenerate__FgenExpr(Anode->Vn_right, Actx, CSymbol__X__Vint);
  Anode->Vn_undefined = (Anode->Vn_left->Vn_undefined + Anode->Vn_right->Vn_undefined);
}
void CResolve__FconcatOp(CResolve *THIS, CNode *Anode, CSContext *Actx) {
  MGenerate__FgenExpr(Anode->Vn_left, Actx, CSymbol__X__Vstring);
  MGenerate__FgenExpr(Anode->Vn_right, Actx, CSymbol__X__Vstring);
  Anode->Vn_undefined = (Anode->Vn_left->Vn_undefined + Anode->Vn_right->Vn_undefined);
}
Zenum CResolve__FplusOp(CResolve *THIS, CNode *Anode, CSContext *Actx, Zenum AdestType) {
  Zenum VretType;
  VretType = 2;
  if ((AdestType != 7))
  {
    CSymbol *Vl;
    Vl = MGenerate__FgenExpr(Anode->Vn_left, CSContext__FcopyNoOut(Actx), NULL);
    CSymbol *Vr;
    Vr = MGenerate__FgenExpr(Anode->Vn_right, CSContext__FcopyNoOut(Actx), NULL);
    if (((AdestType == 0) && (((((Vl == NULL) || (Vl->Vtype == 0)) || (Vr == NULL)) || (Vr->Vtype == 0)))))
    {
      VretType = 0;
      Anode->Vn_undefined = ((Anode->Vn_left->Vn_undefined + Anode->Vn_right->Vn_undefined) + 1);
    }
  }
  if ((VretType == 2))
  {
    VretType = 7;
    CResolve__FnumberOp(THIS, Anode, Actx);
  }
  return VretType;
}
void CResolve__FincrdecrOp(CResolve *THIS, CNode *Anode, CSContext *Actx) {
  if ((Anode->Vn_left->Vn_type == 7))
  {
    CNode__Ferror(Anode, "Cannot increment/decrement constant");
  }
  MGenerate__FgenExpr(Anode->Vn_left, Actx, CSymbol__X__Vint);
  Anode->Vn_undefined = Anode->Vn_left->Vn_undefined;
}
void CResolve__FbooleanOp(CResolve *THIS, CNode *Anode, CSContext *Actx) {
  Anode->Vn_undefined = 0;
  if (((((Anode->Vn_left->Vn_type == 2) || (Anode->Vn_right->Vn_type == 2)) || (Anode->Vn_type == 97)) || (Anode->Vn_type == 98)))
  {
    MGenerate__FgenExpr(Anode->Vn_left, Actx, CSymbol__X__Vnil);
    MGenerate__FgenExpr(Anode->Vn_right, Actx, CSymbol__X__Vnil);
    Anode->Vn_nodeType = 2;
  }
else
  {
    CSymbol *VsymLeft;
    VsymLeft = MGenerate__FgenExpr(Anode->Vn_left, Actx, NULL);
    CSymbol *VsymRight;
    VsymRight = MGenerate__FgenExpr(Anode->Vn_right, Actx, NULL);
    if (((VsymLeft == NULL) || (VsymRight == NULL)))
    {
      ++(Anode->Vn_undefined);
    }
  else
    {
      if (((VsymLeft->Vtype == 9) || (VsymRight->Vtype == 9)))
      {
        MGenerate__FgenExpr(Anode->Vn_left, Actx, CSymbol__X__Vstring);
        MGenerate__FgenExpr(Anode->Vn_right, Actx, CSymbol__X__Vstring);
        Anode->Vn_nodeType = 9;
      }
    else
      {
        VsymLeft = MGenerate__FgenExpr(Anode->Vn_left, Actx, VsymLeft);
        MGenerate__FgenExpr(Anode->Vn_right, Actx, VsymLeft);
        Anode->Vn_nodeType = 24;
        Anode->Vn_symbol = VsymLeft;
        if ((VsymLeft->Vtype == 0))
        {
          ++(Anode->Vn_undefined);
        }
      }
    }
  }
  Anode->Vn_undefined += (Anode->Vn_left->Vn_undefined + Anode->Vn_right->Vn_undefined);
}
void CResolve__FcompareOp(CResolve *THIS, CNode *Anode, CSContext *Actx) {
  MGenerate__FgenExpr(Anode->Vn_left, Actx, CSymbol__X__Vint);
  MGenerate__FgenExpr(Anode->Vn_right, Actx, CSymbol__X__Vint);
  Anode->Vn_undefined = (Anode->Vn_left->Vn_undefined + Anode->Vn_right->Vn_undefined);
}
void CResolve__FandorOp(CResolve *THIS, CNode *Anode, CSContext *Actx) {
  MGenerate__FgenExpr(Anode->Vn_left, Actx, CSymbol__X__Vbool);
  MGenerate__FgenExpr(Anode->Vn_right, Actx, CSymbol__X__Vbool);
  Anode->Vn_undefined = (Anode->Vn_left->Vn_undefined + Anode->Vn_right->Vn_undefined);
}
CSymbol *CResolve__Fparens(CResolve *THIS, CNode *Anode, CSContext *Actx, CSymbol *AdestSym) {
  CSymbol *Vret;
  Vret = MGenerate__FgenExpr(Anode->Vn_left, Actx, AdestSym);
  Anode->Vn_undefined = Anode->Vn_left->Vn_undefined;
  return Vret;
}
CSymbol *CResolve__FaltOp(CResolve *THIS, CNode *Anode, CSContext *Actx, CSymbol *AdestSym) {
  MGenerate__FgenExpr(Anode->Vn_cond, Actx, CSymbol__X__Vbool);
  CSymbol *Vsyml;
  Vsyml = MGenerate__FgenExpr(Anode->Vn_left, Actx, AdestSym);
  CSymbol *Vsymr;
  Vsymr = MGenerate__FgenExpr(Anode->Vn_right, Actx, AdestSym);
  Anode->Vn_undefined = ((Anode->Vn_left->Vn_undefined + Anode->Vn_right->Vn_undefined) + Anode->Vn_cond->Vn_undefined);
  if ((Vsymr != NULL))
  {
    return CSymbol__Fcopy(Vsymr);
  }
  return NULL;
}
CSymbol *CResolve__FlistPart(CResolve *THIS, CNode *Anode, CSContext *Actx, CSymbol *AdestSym) {
  CSymbol *Vsym;
  Vsym = NULL;
  if ((Anode->Vn_type == 0))
  {
    Vsym = AdestSym;
  }
else if ((Anode->Vn_type == 110))
  {
    Vsym = CResolve__FlistPart(THIS, Anode->Vn_left, Actx, AdestSym);
    MGenerate__FgenExpr(Anode->Vn_right, Actx, Vsym);
    Anode->Vn_undefined = (Anode->Vn_left->Vn_undefined + Anode->Vn_right->Vn_undefined);
  }
else
  {
    Vsym = MGenerate__FgenExpr(Anode, Actx, AdestSym);
  }
  return Vsym;
}
void CResolve__FdictPart(CResolve *THIS, CNode *Anode, CSymbol *Aret, CSContext *Actx) {
  if ((Anode->Vn_type == 110))
  {
    CResolve__FdictPart(THIS, Anode->Vn_left, Aret, Actx);
    Anode->Vn_undefined = Anode->Vn_left->Vn_undefined;
  }
else
  {
    Aret->VkeySymbol = MGenerate__FgenExpr(Anode->Vn_cond, Actx, NULL);
    Aret->VreturnSymbol = MGenerate__FgenExpr(Anode->Vn_right, Actx, NULL);
    Anode->Vn_undefined = 0;
  }
  MGenerate__FgenExpr(Anode->Vn_cond, Actx, Aret->VkeySymbol);
  MGenerate__FgenExpr(Anode->Vn_right, Actx, Aret->VreturnSymbol);
  Anode->Vn_undefined += (Anode->Vn_cond->Vn_undefined + Anode->Vn_right->Vn_undefined);
}
CSymbol *CResolve__Fexpr(CResolve *THIS, CNode *Anode, CSContext *Actx, CSymbol *AdestSym) {
  CSymbol *Vret;
  Vret = MGenerate__FgenExprChecked(Anode, Actx, AdestSym);
  if (((AdestSym != NULL) && (Vret != NULL)))
  {
    Zenum Vdest_type;
    Vdest_type = AdestSym->Vtype;
    if (((Vdest_type == 9) && (Vret->Vtype == 7)))
    {
      Anode->Vn_conversion = 1;
      Vret = CSymbol__FNEW(9);
    }
  else if (((Vdest_type == 9) && (Vret->Vtype == 11)))
    {
      Anode->Vn_conversion = 2;
      Vret = CSymbol__FNEW(9);
    }
  else if (((Vdest_type == 9) && (Vret->Vtype == 12)))
    {
      Anode->Vn_conversion = 3;
      Vret = CSymbol__FNEW(9);
    }
  else if (((Vdest_type == 25) && (Vret->Vtype == 24)))
    {
      Anode->Vn_conversion = 4;
      Anode->Vn_ret_class = Vret->Vclass;
      Vret = AdestSym;
    }
  else
    {
      Anode->Vn_conversion = 0;
    }
  }
  return Vret;
}
char *CResolve__FwriteImport(CResolve *THIS, CZimbuFile *Aimport, COutput__X__CGroup *Aouts, CScope *Ascope) {
  MError__Freport("INTERNAL: Resolve.writeImport() should not be called");
  return "";
}
void CResolve__FwriteIncludeImport(CResolve *THIS, CZimbuFile *Aimport, COutput__X__CGroup *AmyOuts, CScope *Ascope) {
  MError__Freport("INTERNAL: Resolve.writeIncludeImport() should not be called");
}
Zbool CResolve__FneedWrite(CResolve *THIS, CZimbuFile *AzimbuFile) {
  return 0;
}
void CResolve__FwriteClassDef(CResolve *THIS, char *Aname, COutput *AtypeOut) {
}
void CResolve__FwriteClassDecl(CResolve *THIS, CSymbol *AclassSym, COutput__X__CGroup *Aouts, COutput *AstructOut) {
}
void CResolve__FdefaultInit(CResolve *THIS, CSymbol *Asym, COutput *Aout) {
}
void CResolve__Fnil(CResolve *THIS, CSContext *Actx) {
}
void CResolve__Fmember(CResolve *THIS, char *Aname, COutput *Aout) {
}
void CResolve__Fvardecl(CResolve *THIS, CSContext *Actx, COutput *Aout) {
}
void CResolve__Fvartype(CResolve *THIS, CSymbol *Asym, CNode *Anode, CScope *Ascope, COutput *Aout) {
}
void CResolve__FforStart(CResolve *THIS, COutput *Aout) {
}
void CResolve__FforLoop(CResolve *THIS, CSymbol *AvarSym, COutput *Aout) {
}
void Iresolve() {
 static int done = 0;
 if (!done) {
  done = 1;
  Idictstuff();
  Igenerate();
  Iliststuff();
  Ioutput();
  Iscontext();
  Iscope();
  Isymbol();
  Izimbufile();
  Iwrite_c();
  Iwrite_js();
 }
}
