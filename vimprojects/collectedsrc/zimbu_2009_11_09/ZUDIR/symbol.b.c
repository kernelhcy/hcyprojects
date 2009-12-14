#define INC_symbol_B 1
/*
 * FUNCTION BODIES
 */
#ifndef INC_attr_B
#include "../ZUDIR/attr.b.c"
#endif
#ifndef INC_generate_B
#include "../ZUDIR/generate.b.c"
#endif
#ifndef INC_node_B
#include "../ZUDIR/node.b.c"
#endif
#ifndef INC_output_B
#include "../ZUDIR/output.b.c"
#endif
#ifndef INC_pos_B
#include "../ZUDIR/pos.b.c"
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
CSymbol *CSymbol__FNEW(Zenum A_type) {
  CSymbol *THIS = Zalloc(sizeof(CSymbol));
  THIS->Vtype = A_type;
  return THIS;
}
char *CSymbol__FtoString(CSymbol *THIS) {
  return CSymbol__FtoString__1(THIS, "", 1);
}
char *CSymbol__FtoString__1(CSymbol *THIS, char *Aindent, Zbool Arecurse) {
  char *Vres;
  Vres = Zconcat(Aindent, "Symbol");
  if ((THIS->Vname != NULL))
  {
    Vres = Zconcat(Vres, Zconcat(" name: ", THIS->Vname));
  }
  Vres = Zconcat(Vres, Zconcat(Zconcat(" type: ", Zenum2string(CNode__X__EType, sizeof(CNode__X__EType) / sizeof(char *), THIS->Vtype)), "\n"));
  if ((THIS->Vclass != NULL))
  {
    Vres = Zconcat(Vres, Zconcat(Zconcat(Zconcat(Aindent, "  class: "), THIS->Vclass->Vname), "\n"));
  }
  if ((THIS->VparentClass != NULL))
  {
    Vres = Zconcat(Vres, Zconcat(Zconcat(Zconcat(Aindent, "  parent: "), THIS->VparentClass->Vname), "\n"));
  }
  if ((Arecurse && (THIS->VreturnSymbol != NULL)))
  {
    Vres = Zconcat(Vres, Zconcat(Zconcat(Aindent, "  returnSymbol: "), CSymbol__FtoString__1(THIS->VreturnSymbol, Zconcat(Aindent, "    "), 0)));
  }
  if ((Arecurse && (THIS->VmemberList != NULL)))
  {
    Vres = Zconcat(Vres, Zconcat(Aindent, "  members:\n"));
    {
      Zfor_T *Zf = ZforNew(THIS->VmemberList, 2);
      CSymbol *Vm;
      for (ZforGetPtr(Zf, (char **)&Vm); ZforCont(Zf); ZforNextPtr(Zf, (char **)&Vm)) {
        if ((Vm == NULL))
        {
          Vres = Zconcat(Vres, Zconcat(Aindent, "    NIL\n"));
        }
      else
        {
          Vres = Zconcat(Vres, CSymbol__FtoString__1(Vm, Zconcat(Aindent, "    "), 0));
        }
      }
    }
  }
  if (Arecurse)
  {
    if ((THIS->Vchildren != NULL))
    {
      Vres = Zconcat(Vres, Zconcat(Aindent, "  children:\n"));
      {
        Zfor_T *Zf = ZforNew(THIS->Vchildren, 2);
        CSymbol *Vm;
        for (ZforGetPtr(Zf, (char **)&Vm); ZforCont(Zf); ZforNextPtr(Zf, (char **)&Vm)) {
          if ((Vm == NULL))
          {
            Vres = Zconcat(Vres, Zconcat(Aindent, "    NIL\n"));
          }
        else
          {
            Vres = Zconcat(Vres, CSymbol__FtoString__1(Vm, Zconcat(Aindent, "    "), 0));
          }
        }
      }
    }
  else
    {
      Vres = Zconcat(Vres, Zconcat(Aindent, "  no children\n"));
    }
  }
  return Vres;
}
char *CSymbol__FdumpSymbols(CSymbol *THIS) {
  char *Vs;
  Vs = Zconcat(Zenum2string(CNode__X__EType, sizeof(CNode__X__EType) / sizeof(char *), THIS->Vtype), " { ");
  Vs = Zconcat(Vs, Zconcat("name : ", THIS->Vname));
  Vs = Zconcat(Vs, " }");
  return Vs;
}
void CSymbol__FaddFromSymbol(CSymbol *THIS, char *Adump) {
  Zint Vidx;
  Vidx = 0;
  Zint Vlen;
  Vlen = strlen(Adump);
  while ((Vidx < Vlen))
  {
    char *Vhead;
    Vhead = ZStringSlice(Adump, Vidx, (Vidx + 20));
    Zint Vw;
    Vw = ZStringIndex(Vhead, 32);
    if ((Vw < 0))
    {
      break;
    }
    char *VsymType;
    VsymType = ZStringSlice(Vhead, 0, Vw);
    break;
  }
}
CSymbol *CSymbol__Fcopy(CSymbol *THIS) {
  CSymbol *Vret;
  Vret = CSymbol__FNEW(THIS->Vtype);
  Vret->Vname = THIS->Vname;
  Vret->VcName = THIS->VcName;
  Vret->Vvalue = THIS->Vvalue;
  Vret->Vmask = THIS->Vmask;
  Vret->Vused = THIS->Vused;
  Vret->VmemberList = THIS->VmemberList;
  Vret->Vchildren = THIS->Vchildren;
  Vret->Vclass = THIS->Vclass;
  Vret->VclassName = THIS->VclassName;
  Vret->VparentClass = THIS->VparentClass;
  Vret->Vinterfaces = THIS->Vinterfaces;
  Vret->VkeySymbol = THIS->VkeySymbol;
  Vret->VreturnSymbol = THIS->VreturnSymbol;
  Vret->Vattributes = THIS->Vattributes;
  if ((THIS->Vpos != NULL))
  {
    Vret->Vpos = CPos__Fcopy(THIS->Vpos);
  }
  Vret->Vproduce = THIS->Vproduce;
  return Vret;
}
CSymbol *CSymbol__FcopyObject(CSymbol *THIS) {
  CSymbol *Vret;
  Vret = CSymbol__Fcopy(THIS);
  if ((THIS->Vtype == 21))
  {
    Vret->Vtype = 24;
    Vret->Vclass = THIS;
  }
  return Vret;
}
CSymbol *CSymbol__FaddMember(CSymbol *THIS, char *A_name, CSymbol *AtypeSym, Zint A_value) {
  CSymbol *Vs;
  Vs = NULL;
  if ((AtypeSym == NULL))
  {
    Vs = CSymbol__FNEW(0);
  }
else
  {
    Vs = CSymbol__Fcopy(AtypeSym);
  }
  Vs->Vname = A_name;
  Vs->Vvalue = A_value;
  return CSymbol__FaddMember__1(THIS, Vs);
}
CSymbol *CSymbol__FaddMember__1(CSymbol *THIS, CSymbol *AtypeSym) {
  if ((THIS->VmemberList == NULL))
  {
    THIS->VmemberList = Zalloc(sizeof(CListHead));
  }
  ZListAdd(THIS->VmemberList, -1, 0, AtypeSym, 1);
  return AtypeSym;
}
void CSymbol__FremoveMember(CSymbol *THIS, CSymbol *Asym) {
  if ((THIS->VmemberList != NULL))
  {
    Zint Vi;
    Vi = 0;
    while ((Vi < THIS->VmemberList->itemCount))
    {
      if ((((CSymbol *)ZListGetPtr(THIS->VmemberList, Vi)) == Asym))
      {
        ZListPopPtrItem(THIS->VmemberList, Vi);
        return ;
      }
      ++(Vi);
    }
  }
}
Zbool CSymbol__FhasMember(CSymbol *THIS) {
  return ((THIS->VmemberList != NULL) && (THIS->VmemberList->itemCount > 0));
}
void CSymbol__FaddChild(CSymbol *THIS, CSymbol *Achild) {
  if ((THIS->Vchildren == NULL))
  {
    THIS->Vchildren = Zalloc(sizeof(CListHead));
  }
else
  {
    {
      Zfor_T *Zf = ZforNew(THIS->Vchildren, 2);
      CSymbol *Vs;
      for (ZforGetPtr(Zf, (char **)&Vs); ZforCont(Zf); ZforNextPtr(Zf, (char **)&Vs)) {
        if ((Vs == Achild))
        {
          return ;
        }
      }
    }
  }
  ZListAdd(THIS->Vchildren, -1, 0, Achild, 1);
}
void CSymbol__FaddInterface(CSymbol *THIS, CSymbol *Aitf, CNode *A_node) {
  if ((THIS->Vinterfaces == NULL))
  {
    THIS->Vinterfaces = Zalloc(sizeof(CListHead));
  }
else
  {
    {
      Zfor_T *Zf = ZforNew(THIS->Vinterfaces, 2);
      CSymbol *Vs;
      for (ZforGetPtr(Zf, (char **)&Vs); ZforCont(Zf); ZforNextPtr(Zf, (char **)&Vs)) {
        if ((Vs == Aitf))
        {
          CNode__Ferror(A_node, "Duplicate interface");
          return ;
        }
      }
    }
  }
  ZListAdd(THIS->Vinterfaces, -1, 0, Aitf, 1);
}
Zint CSymbol__FfindChild(CSymbol *THIS, CSymbol *Achild) {
  Zint Vidx;
  Vidx = 0;
  if ((THIS->Vchildren != NULL))
  {
    {
      Zfor_T *Zf = ZforNew(THIS->Vchildren, 2);
      CSymbol *Vs;
      for (ZforGetPtr(Zf, (char **)&Vs); ZforCont(Zf); ZforNextPtr(Zf, (char **)&Vs)) {
        if ((Vs == Achild))
        {
          return Vidx;
        }
        if (!((((Vs->Vattributes) & 1))))
        {
          ++(Vidx);
        }
        if ((Vs->Vchildren != NULL))
        {
          Zint Vci;
          Vci = CSymbol__FfindChild(Vs, Achild);
          if ((Vci >= 0))
          {
            return (Vidx + Vci);
          }
          Vidx += -(((Vci + 1)));
        }
      }
    }
  }
  return -(((Vidx + 1)));
}
Zint CSymbol__FchildIndex(CSymbol *THIS, CSymbol *Achild) {
  if ((Achild == NULL))
  {
    return 0;
  }
  if ((Achild == THIS))
  {
    return 0;
  }
  Zint Vi;
  Vi = CSymbol__FfindChild(THIS, Achild);
  if ((Vi >= 0))
  {
    return (Vi + (((((THIS->Vattributes) & 1))) ? (0) : (1)));
  }
  return -(1);
}
CSymbol *CSymbol__FaddLibMethod(CSymbol *THIS, char *A_name, void *AproduceProc, CSymbol *A_returnSymbol) {
  CSymbol *Vmember;
  Vmember = CSymbol__FaddMember(THIS, A_name, CSymbol__FNEW(111), 0);
  Vmember->Vproduce = AproduceProc;
  Vmember->VreturnSymbol = A_returnSymbol;
  return Vmember;
}
CListHead *CSymbol__FgetMemberList(CSymbol *THIS) {
  if (((THIS->Vtype == 6) && (THIS->VreturnSymbol != NULL)))
  {
    return CSymbol__FgetMemberList(THIS->VreturnSymbol);
  }
  if (((((THIS->Vtype == 21) || (THIS->Vtype == 24))) && (THIS->Vclass != NULL)))
  {
    return THIS->Vclass->VmemberList;
  }
  return THIS->VmemberList;
}
CSymbol *CSymbol__FfindMember(CSymbol *THIS, char *A_name) {
  CSymbol *Vs;
  Vs = CSymbol__X__Ffind(CSymbol__FgetMemberList(THIS), A_name);
  if ((((Vs == NULL) && (((THIS->Vtype == 24) || (THIS->Vtype == 21)))) && (THIS->VparentClass != NULL)))
  {
    return CSymbol__FfindMember(THIS->VparentClass, A_name);
  }
  return Vs;
}
CSymbol *CSymbol__FfindMatchingMember(CSymbol *THIS, CSymbol *Amember) {
  if ((THIS->VmemberList == NULL))
  {
    return NULL;
  }
  {
    Zfor_T *Zf = ZforNew(THIS->VmemberList, 2);
    CSymbol *Vm;
    for (ZforGetPtr(Zf, (char **)&Vm); ZforCont(Zf); ZforNextPtr(Zf, (char **)&Vm)) {
      if ((Zstrcmp(Vm->Vname, Amember->Vname) == 0))
      {
        if (((Vm->VmemberList == NULL) && (Amember->VmemberList == NULL)))
        {
          return Vm;
        }
        Zint Vidx;
        Vidx = 0;
        if ((Amember->VmemberList != NULL))
        {
          while ((Vidx < Vm->VmemberList->itemCount))
          {
            if (!(MGenerate__FcompatibleSymbols(((CSymbol *)ZListGetPtr(Amember->VmemberList, Vidx)), ((CSymbol *)ZListGetPtr(Vm->VmemberList, Vidx)))))
            {
              break;
            }
            ++(Vidx);
          }
        }
        if ((Vidx == Vm->VmemberList->itemCount))
        {
          return Vm;
        }
      }
    }
  }
  return NULL;
}
CSymbol *CSymbol__FfindMember__1(CSymbol *THIS, char *A_name, Zenum AtypeReq) {
  CSymbol *Vs;
  Vs = CSymbol__X__Ffind__1(CSymbol__FgetMemberList(THIS), A_name, AtypeReq);
  if ((((Vs == NULL) && (((THIS->Vtype == 24) || (THIS->Vtype == 21)))) && (THIS->VparentClass != NULL)))
  {
    return CSymbol__FfindMember__1(THIS->VparentClass, A_name, AtypeReq);
  }
  return Vs;
}
char *CSymbol__FgetClassName(CSymbol *THIS) {
  if (((THIS->Vclass != NULL) && (THIS->Vclass->VclassName != NULL)))
  {
    return THIS->Vclass->VclassName;
  }
  return THIS->VclassName;
}
Zbool CSymbol__Fmatches(CSymbol *THIS, char *A_name, Zenum A_type) {
  return ((Zstrcmp(THIS->Vname, A_name) == 0) && (((A_type == 0) || (THIS->Vtype == A_type))));
}
CSymbol *CSymbol__FfindMatchingFunction(CSymbol *THIS, char *Amethod, CListHead *AargList, CSymbol *Askip, Zbool AsearchParent, Zbool Aconvert) {
  Zbool VfoundSkip;
  VfoundSkip = 0;
  CSymbol *Vmatch;
  Vmatch = CSymbol__X__FfindMatchingFunctionInList(THIS->VmemberList, Amethod, AargList, Askip, &(VfoundSkip), Aconvert);
  if ((((((Vmatch == NULL) && (((THIS->Vtype == 24) || (THIS->Vtype == 21)))) && AsearchParent) && !(VfoundSkip)) && (THIS->VparentClass != NULL)))
  {
    return CSymbol__FfindMatchingFunction(THIS->VparentClass, Amethod, AargList, Askip, 1, Aconvert);
  }
  return Vmatch;
}
Zenum CSymbol__FgetReturnType(CSymbol *THIS) {
  if ((THIS->VreturnSymbol == NULL))
  {
    return 0;
  }
  return THIS->VreturnSymbol->Vtype;
}
Zbool CSymbol__FisPointerType(CSymbol *THIS) {
  return CNode__X__FisPointerType(THIS->Vtype);
}
Zbool CSymbol__FisMethodType(CSymbol *THIS) {
  return CNode__X__FisMethodType(THIS->Vtype);
}
CSymbol *CSymbol__X__Ffind(CListHead *Alist, char *Aname) {
  if ((Alist != NULL))
  {
    {
      Zfor_T *Zf = ZforNew(Alist, 2);
      CSymbol *Vsym;
      for (ZforGetPtr(Zf, (char **)&Vsym); ZforCont(Zf); ZforNextPtr(Zf, (char **)&Vsym)) {
        if ((Zstrcmp(Vsym->Vname, Aname) == 0))
        {
          Vsym->Vused = 1;
          return Vsym;
        }
      }
    }
  }
  return NULL;
}
CSymbol *CSymbol__X__Ffind__1(CListHead *Alist, char *Aname, Zenum Atype) {
  if ((Alist != NULL))
  {
    {
      Zfor_T *Zf = ZforNew(Alist, 2);
      CSymbol *Vsym;
      for (ZforGetPtr(Zf, (char **)&Vsym); ZforCont(Zf); ZforNextPtr(Zf, (char **)&Vsym)) {
        if (CSymbol__Fmatches(Vsym, Aname, Atype))
        {
          Vsym->Vused = 1;
          return Vsym;
        }
      }
    }
  }
  return NULL;
}
CSymbol *CSymbol__X__FfindMatchingFunctionInList(CListHead *AmemberList, char *Amethod, CListHead *AargList, CSymbol *Askip, Zbool  *RfoundSkip, Zbool Aconvert) {
  if ((AmemberList == NULL))
  {
    return NULL;
  }
  Zint VargListSize;
  VargListSize = 0;
  if ((AargList != NULL))
  {
    VargListSize = AargList->itemCount;
  }
  {
    Zfor_T *Zf = ZforNew(AmemberList, 2);
    CSymbol *Vsym;
    for (ZforGetPtr(Zf, (char **)&Vsym); ZforCont(Zf); ZforNextPtr(Zf, (char **)&Vsym)) {
      if ((Vsym == Askip))
      {
        (*RfoundSkip) = 1;
      }
    else if ((Zstrcmp(Vsym->Vname, Amethod) == 0))
      {
        Zint Vsize;
        Vsize = 0;
        if ((Vsym->VmemberList != NULL))
        {
          Vsize = Vsym->VmemberList->itemCount;
        }
        if ((Vsize == VargListSize))
        {
          Zint Vidx;
          Vidx = 0;
          while ((Vidx < Vsize))
          {
            CSymbol *Va1;
            Va1 = ((CSymbol *)ZListGetPtr(Vsym->VmemberList, Vidx));
            CSymbol *Va2;
            Va2 = ((CSymbol *)ZListGetPtr(AargList, Vidx));
            if ((Va2 == NULL))
            {
              break;
            }
            if (((Va1->Vtype == 6) && (Va1->VreturnSymbol != NULL)))
            {
              Va1 = Va1->VreturnSymbol;
            }
            if (((Va2->Vtype == 6) && (Va2->VreturnSymbol != NULL)))
            {
              Va2 = Va2->VreturnSymbol;
            }
            if (!(((((((((((((Va1->Vtype == Va2->Vtype) && (((((Va1->Vtype != 24) && (Va1->Vtype != 21))) || (Va1->Vclass == Va2->Vclass)))) || ((((Va1->Vtype == 21) && (Va2->Vtype == 24)) && (Va1->Vclass == Va2->Vclass)))) || (Va1->Vtype == 17)) || (Va2->Vtype == 0)) || (((((Va1->Vtype == 26) || (Va1->Vtype == 27))) && (Va2->Vtype == 7)))) || (((Va1->Vtype == 36) && (Va2->Vtype == 34)))) || (((Va1->Vtype == 34) && (Va2->Vtype == 36)))) || (((Va1->Vtype == 37) && (Va2->Vtype == 35)))) || (((Va1->Vtype == 35) && (Va2->Vtype == 37)))) || ((Aconvert && MGenerate__FcompatibleSymbols(Va2, Va1)))))))
            {
              break;
            }
            ++(Vidx);
          }
          if ((Vidx == Vsize))
          {
            Vsym->Vused = 1;
            return Vsym;
          }
        }
      }
    }
  }
  return NULL;
}
void Isymbol() {
 static int done = 0;
 if (!done) {
  done = 1;
  Igenerate();
  Ioutput();
  Iresolve();
  Iscontext();
  Iscope();
  CSymbol__X__Vunknown = CSymbol__FNEW(0);
  CSymbol__X__Vbool = CSymbol__FNEW(11);
  CSymbol__X__Vstatus = CSymbol__FNEW(12);
  CSymbol__X__Vint = CSymbol__FNEW(7);
  CSymbol__X__Vstring = CSymbol__FNEW(9);
  CSymbol__X__Vparent = CSymbol__FNEW(4);
  CSymbol__X__Varray = CSymbol__FNEW(16);
  CSymbol__X__Vnil = CSymbol__FNEW(2);
  CSymbol__X__Vproc_ref = CSymbol__FNEW(37);
  CSymbol__X__Vfunc_ref = CSymbol__FNEW(36);
 }
}
