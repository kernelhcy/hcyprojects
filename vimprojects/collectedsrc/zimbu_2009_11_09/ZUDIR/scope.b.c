#define INC_scope_B 1
/*
 * FUNCTION BODIES
 */
#ifndef INC_builtin_B
#include "../ZUDIR/builtin.b.c"
#endif
#ifndef INC_error_B
#include "../ZUDIR/error.b.c"
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
#ifndef INC_symbol_B
#include "../ZUDIR/symbol.b.c"
#endif
#ifndef INC_tokenize_B
#include "../ZUDIR/tokenize.b.c"
#endif
#ifndef INC_usedfile_B
#include "../ZUDIR/usedfile.b.c"
#endif
#ifndef INC_zimbufile_B
#include "../ZUDIR/zimbufile.b.c"
#endif
#ifndef INC_libarg_B
#include "../lib/ZUDIR/libarg.b.c"
#endif
#ifndef INC_libio_B
#include "../lib/ZUDIR/libio.b.c"
#endif
#ifndef INC_libsys_B
#include "../lib/ZUDIR/libsys.b.c"
#endif
#ifndef INC_libthread_B
#include "../lib/ZUDIR/libthread.b.c"
#endif
Zbool CScope__FisClassScope(CScope *THIS) {
  return ((THIS->Vclass != NULL) && !(THIS->VinsideShared));
}
CSymbol *CScope__FgetSymbol(CScope *THIS, char *Aname) {
  return CScope__FgetSymbol__3(THIS, Aname, 0, 1, NULL);
}
CSymbol *CScope__FgetSymbol__1(CScope *THIS, char *Aname, Zbool AclassCheck) {
  return CScope__FgetSymbol__3(THIS, Aname, 0, AclassCheck, NULL);
}
CSymbol *CScope__FgetSymbol__2(CScope *THIS, char *Aname, CNode *Anode) {
  return CScope__FgetSymbol__3(THIS, Aname, 0, 1, Anode);
}
CSymbol *CScope__FgetSymbol__3(CScope *THIS, char *Aname, Zenum Atype, Zbool AclassCheck, CNode *Anode) {
  CSymbol *Vsym;
  Vsym = NULL;
  if ((Zstrcmp(Aname, THIS->VskipPredefined) != 0))
  {
    Vsym = CScope__X__FfindPredefinedSymbol(Aname, Atype);
  }
  if ((Vsym == NULL))
  {
    Vsym = CScope__FgetSymbolInt(THIS, Aname, Atype, THIS->VinsideShared, AclassCheck, Anode);
  }
  return Vsym;
}
CSymbol *CScope__FgetSymbolInt(CScope *THIS, char *Aname, Zenum Atype, Zbool AinShared, Zbool AclassCheck, CNode *Anode) {
  CSymbol *Vsym;
  Vsym = CSymbol__X__Ffind__1(THIS->VmemberList, Aname, Atype);
  if ((((((Vsym != NULL) && AclassCheck) && AinShared) && (THIS->Vclass != NULL)) && (((Vsym->VclassName != NULL) && (Vsym->Vtype != 21)))))
  {
    Vsym = NULL;
    if ((Anode != NULL))
    {
      CNode__Ferror(Anode, "Cannot use class member in SHARED section");
    }
  }
  if ((((Vsym == NULL) && CScope__FisClassScope(THIS)) && (THIS->Vclass->VparentClass != NULL)))
  {
    Vsym = CSymbol__FfindMember__1(THIS->Vclass->VparentClass, Aname, Atype);
  }
  if (((Vsym == NULL) && (THIS->VimportList != NULL)))
  {
    {
      Zfor_T *Zf = ZforNew(THIS->VimportList, 2);
      CZimbuFile *Vimport;
      for (ZforGetPtr(Zf, (char **)&Vimport); ZforCont(Zf); ZforNextPtr(Zf, (char **)&Vimport)) {
        if ((Vimport->VtopScope != NULL))
        {
          Vsym = CSymbol__X__Ffind__1(Vimport->VtopScope->VmemberList, Aname, Atype);
          if ((Vsym != NULL))
          {
            break;
          }
        }
      }
    }
  }
  if ((Vsym != NULL))
  {
    return Vsym;
  }
  if ((THIS->Vouter != NULL))
  {
    return CScope__FgetSymbolInt(THIS->Vouter, Aname, Atype, AinShared, AclassCheck, Anode);
  }
  return NULL;
}
Zbool CScope__FusedAsZwt(CScope *THIS) {
  if ((THIS->VzimbuFile != NULL))
  {
    return THIS->VzimbuFile->VusedAsZwt;
  }
  if ((THIS->Vouter != NULL))
  {
    return CScope__FusedAsZwt(THIS->Vouter);
  }
  return 0;
}
Zbool CScope__FusedAsZimbu(CScope *THIS) {
  if ((THIS->VzimbuFile != NULL))
  {
    return THIS->VzimbuFile->VusedAsZimbu;
  }
  if ((THIS->Vouter != NULL))
  {
    return CScope__FusedAsZimbu(THIS->Vouter);
  }
  return 0;
}
CSymbol *CScope__FfindNodeSymbol(CScope *THIS, CNode *Anode) {
  return CScope__FfindNodeSymbol__1(THIS, Anode, 0, 0);
}
CSymbol *CScope__FfindNodeSymbol__1(CScope *THIS, CNode *Anode, Zenum Atype, Zbool Aerror) {
  switch (Anode->Vn_type)
  {
  case 5:
    {
      {
        CSymbol *Vsym;
        Vsym = CScope__FgetSymbol__3(THIS, Anode->Vn_string, Atype, 1, Anode);
        if ((Vsym == NULL))
        {
          ++(Anode->Vn_undefined);
          if (Aerror)
          {
            CNode__Ferror(Anode, Zconcat("Symbol not found: ", Anode->Vn_string));
          }
        }
      else if (((Vsym->Vtype == 6) && (Vsym->VreturnSymbol != NULL)))
        {
          Vsym = Vsym->VreturnSymbol;
        }
        return Vsym;
      }
        break;
    }
  case 105:
    {
      {
        CSymbol *Vsym;
        Vsym = CScope__FfindNodeSymbol__1(THIS, Anode->Vn_left, Atype, Aerror);
        if ((Vsym != NULL))
        {
          CSymbol *Vs;
          Vs = NULL;
          if ((Zstrcmp(Anode->Vn_string, "I") == 0))
          {
            if (((Vsym->Vtype == 19) && (CSymbol__FgetMemberList(Vsym) != NULL)))
            {
              CSymbol *Vcs;
              Vcs = CSymbol__X__Ffind__1(CSymbol__FgetMemberList(Vsym), Anode->Vn_left->Vn_string, 21);
              if ((Vcs != NULL))
              {
                Vsym = Vcs;
              }
            }
            if ((Vsym->Vtype != 21))
            {
              ++(Anode->Vn_undefined);
              if (Aerror)
              {
                CNode__Ferror(Anode->Vn_left, "Class expected");
              }
            }
            Vs = CSymbol__FNEW(25);
            Vs->Vclass = Vsym;
          }
        else
          {
            Vs = CSymbol__FfindMember(Vsym, Anode->Vn_string);
            if ((Vs == NULL))
            {
              ++(Anode->Vn_undefined);
              if (Aerror)
              {
                if ((CSymbol__FgetMemberList(Vsym) == NULL))
                {
                  CNode__Ferror(Anode, Zconcat("Symbol has no members: ", Anode->Vn_left->Vn_string));
                }
              else
                {
                  CNode__Ferror(Anode, Zconcat(Zconcat(Zconcat("Member ", Anode->Vn_string), " not found in "), Anode->Vn_left->Vn_string));
                }
              }
            }
          }
          return Vs;
        }
      }
        break;
    }
  default:
    {
      {
        ++(Anode->Vn_undefined);
        if (Aerror)
        {
          CNode__Ferror(Anode, Zconcat("Unexpected node type: ", Zenum2string(CNode__X__EType, sizeof(CNode__X__EType) / sizeof(char *), Anode->Vn_type)));
        }
      }
        break;
    }
  }
  return NULL;
}
void CScope__FaddImport(CScope *THIS, CZimbuFile *Aimport) {
  if ((THIS->VimportList == NULL))
  {
    THIS->VimportList = Zalloc(sizeof(CListHead));
  }
  ZListAdd(THIS->VimportList, -1, 0, Aimport, 1);
}
Zenum CScope__FgetSymbolType(CScope *THIS, char *Aname) {
  CSymbol *Vsym;
  Vsym = CScope__FgetSymbol(THIS, Aname);
  if ((Vsym != NULL))
  {
    return Vsym->Vtype;
  }
  return 0;
}
Zenum CScope__FgetAType(CScope *THIS, char *Aname) {
  Zenum Vtype;
  Vtype = CNode__X__Fname2Type(Aname);
  if ((Vtype == 0))
  {
    Vtype = CScope__FgetSymbolType(THIS, Aname);
  }
  return Vtype;
}
CSymbol *CScope__FaddSymbol(CScope *THIS, char *Aname, Zenum Atype, CNode *Anode, Zbool AdupOk) {
  CSymbol *VnewSym;
  VnewSym = NULL;
  Zbool VdoAdd;
  VdoAdd = 1;
  CSymbol *VoldSym;
  VoldSym = NULL;
  if (!(AdupOk))
  {
    if (((((Zstrcmp(Aname, "NEW") == 0) || (Zstrcmp(Aname, "SIZE") == 0)) || (Zstrcmp(Aname, "EQUAL") == 0)) || ((((Atype == 21) && (THIS->VmoduleName != NULL)) && (Zstrcmp(Aname, THIS->VmoduleName) == 0)))))
    {
      VoldSym = CSymbol__X__Ffind(THIS->VmemberList, Aname);
    }
  else
    {
      VoldSym = CScope__FgetSymbol(THIS, Aname);
    }
  }
  if (((THIS->Vpass > 1) && THIS->VforwardDeclare))
  {
    if ((VoldSym == NULL))
    {
      CNode__Ferror(Anode, Zconcat("INTERNAL: Symbol not found in second round: ", Aname));
    }
  else
    {
      VnewSym = VoldSym;
      VdoAdd = 0;
    }
  }
else
  {
    if ((VoldSym != NULL))
    {
      if ((VoldSym->Vpos != NULL))
      {
        CNode__Ferror(Anode, Zconcat(Zconcat(Zconcat("Symbol already defined in line ", Zint2string(VoldSym->Vpos->Vlnum)), ": "), Aname));
      }
    else
      {
        CNode__Ferror(Anode, Zconcat("Duplicate Symbol: ", Aname));
      }
    }
  else if ((!(THIS->VforwardDeclare) && (Anode->Vn_symbol != NULL)))
    {
      VnewSym = Anode->Vn_symbol;
    }
  }
  if ((VnewSym == NULL))
  {
    VnewSym = CSymbol__FNEW(Atype);
  }
else
  {
    VnewSym->Vtype = Atype;
  }
  VnewSym->Vname = Aname;
  VnewSym->Vpos = Anode->Vn_start;
  if (VdoAdd)
  {
    if ((THIS->VmemberList == NULL))
    {
      THIS->VmemberList = Zalloc(sizeof(CListHead));
    }
    ZListInsert(THIS->VmemberList, 0, 0, VnewSym, 1);
  }
  return VnewSym;
}
void CScope__FaddMember(CScope *THIS, CSymbol *Asym) {
  if ((THIS->VmemberList == NULL))
  {
    THIS->VmemberList = Zalloc(sizeof(CListHead));
  }
  ZListAdd(THIS->VmemberList, -1, 0, Asym, 1);
}
CSymbol *CScope__FaddSymbol__1(CScope *THIS, char *Aname, CSymbol *Asp, CNode *Anode, Zbool AdupOk) {
  CSymbol *Vsym;
  Vsym = CScope__FaddSymbol(THIS, Aname, ((Asp == NULL)) ? (0) : (Asp->Vtype), Anode, AdupOk);
  if ((!(CScope__FisClassScope(THIS)) || THIS->Vstatements))
  {
    Vsym->VclassName = NULL;
  }
else
  {
    Vsym->VclassName = THIS->Vclass->VclassName;
  }
  if ((Asp != NULL))
  {
    Vsym->VmemberList = Asp->VmemberList;
    Vsym->Vchildren = Asp->Vchildren;
    Vsym->Vclass = Asp->Vclass;
    Vsym->VparentClass = Asp->VparentClass;
    Vsym->VkeySymbol = Asp->VkeySymbol;
    Vsym->VreturnSymbol = Asp->VreturnSymbol;
  }
  return Vsym;
}
CSymbol *CScope__FfindMatchingFunc(CScope *THIS, char *Aname, CListHead *AargList, CSymbol *Askip, Zbool AsearchOuter, Zbool Aconvert) {
  if (MError__Vdebug)
  {
    (fputs(Zconcat("findMatchingFunc() ", Aname), stdout) | fputc('\n', stdout));
  }
  Zbool VfoundSkip;
  VfoundSkip = 0;
  CSymbol *Vsym;
  Vsym = CSymbol__X__FfindMatchingFunctionInList(THIS->VmemberList, Aname, AargList, Askip, &(VfoundSkip), Aconvert);
  if ((((Vsym == NULL) && AsearchOuter) && (THIS->Vouter != NULL)))
  {
    Vsym = CScope__FfindMatchingFunc(THIS->Vouter, Aname, AargList, Askip, 1, Aconvert);
  }
  return Vsym;
}
void CScope__FlistMatchingFunc(CScope *THIS, char *Aname, CListHead *AargList, Zbool Aall) {
  if ((THIS->VmemberList != NULL))
  {
    {
      Zfor_T *Zf = ZforNew(THIS->VmemberList, 2);
      CSymbol *Vm;
      for (ZforGetPtr(Zf, (char **)&Vm); ZforCont(Zf); ZforNextPtr(Zf, (char **)&Vm)) {
        if ((Zstrcmp(Vm->Vname, Aname) == 0))
        {
          fputs(Zconcat("Candidate: ", Aname), stdout);
          MGenerate__FlistArgTypes(Vm->VmemberList);
        }
      }
    }
  }
  if ((THIS->Vouter != NULL))
  {
    CScope__FlistMatchingFunc(THIS->Vouter, Aname, AargList, Aall);
  }
}
Zenum CScope__FgetReturnType(CScope *THIS) {
  if ((THIS->VreturnSymbol == NULL))
  {
    return 0;
  }
  return THIS->VreturnSymbol->Vtype;
}
void CScope__FaddUsedItem(CScope *THIS, char *Aname) {
  if ((THIS->VusedItems == NULL))
  {
    if ((THIS->Vouter != NULL))
    {
      CScope__FaddUsedItem(THIS->Vouter, Aname);
    }
  else
    {
      MError__Freport("INTERNAL: can't find usedItems");
    }
  }
else
  {
    ZDictAdd(1, THIS->VusedItems, 0, Aname, 1, NULL);
  }
}
void CScope__FmergeKeywords(CScope *THIS, CScope *Ascope) {
  if ((THIS->VusedIdKeywords == NULL))
  {
    if ((THIS->Vouter != NULL))
    {
      CScope__FmergeKeywords(THIS->Vouter, Ascope);
    }
  else
    {
      MError__Freport("INTERNAL: can't find usedIdKeywords");
    }
  }
else
  {
    {
      Zfor_T *Zf = ZforNew(ZDictKeys(Ascope->VusedIdKeywords), 2);
      char *Vkey;
      for (ZforGetPtr(Zf, (char **)&Vkey); ZforCont(Zf); ZforNextPtr(Zf, (char **)&Vkey)) {
        if (!(ZDictHas(THIS->VusedIdKeywords, 0, Vkey)))
        {
          ZDictAdd(1, THIS->VusedIdKeywords, 0, Vkey, 1, NULL);
        }
      }
    }
    {
      Zfor_T *Zf = ZforNew(ZDictKeys(Ascope->VusedItems), 2);
      char *Vkey;
      for (ZforGetPtr(Zf, (char **)&Vkey); ZforCont(Zf); ZforNextPtr(Zf, (char **)&Vkey)) {
        if (!(ZDictHas(THIS->VusedItems, 0, Vkey)))
        {
          ZDictAdd(1, THIS->VusedItems, 0, Vkey, 1, NULL);
        }
      }
    }
  }
}
CSymbol *CScope__X__FfindPredefinedSymbol(char *Aname, Zenum Atype) {
  if ((CScope__X__VpredefinedSymbols == NULL))
  {
    CScope__X__FloadPredefinedSymbols();
  }
  {
    Zfor_T *Zf = ZforNew(CScope__X__VpredefinedSymbols, 2);
    CSymbol *Vsym;
    for (ZforGetPtr(Zf, (char **)&Vsym); ZforCont(Zf); ZforNextPtr(Zf, (char **)&Vsym)) {
      if (CSymbol__Fmatches(Vsym, Aname, Atype))
      {
        return Vsym;
      }
    }
  }
  return NULL;
}
void CScope__X__FloadPredefinedSymbols() {
  CScope__X__VpredefinedSymbols = Zalloc(sizeof(CListHead));
  ZListAdd(CScope__X__VpredefinedSymbols, -1, 0, MLibARG__FgetSymbol(), 1);
  ZListAdd(CScope__X__VpredefinedSymbols, -1, 0, MLibIO__FgetSymbol(), 1);
  ZListAdd(CScope__X__VpredefinedSymbols, -1, 0, MLibSYS__FgetSymbol(), 1);
  ZListExtend(CScope__X__VpredefinedSymbols, MLibTHREAD__FgetSymbols());
}
void CScope__X__FaddPredefinedSymbol(CSymbol *Asym) {
  if ((CScope__X__VpredefinedSymbols == NULL))
  {
    CScope__X__FloadPredefinedSymbols();
  }
  ZListAdd(CScope__X__VpredefinedSymbols, -1, 0, Asym, 1);
}
CScope *CScope__X__FnewScope(CScope *Aouter, Zbool AforwardDeclare) {
  CScope *Vscope;
  Vscope = Zalloc(sizeof(CScope));
  if ((Aouter != NULL))
  {
    Vscope->Vouter = Aouter;
    Vscope->Vdepth = (Aouter->Vdepth + 1);
    Vscope->VreturnSymbol = Aouter->VreturnSymbol;
    Vscope->Vstatements = 1;
    Vscope->Vpass = Aouter->Vpass;
    Vscope->Vclass = Aouter->Vclass;
    Vscope->VinsideNew = Aouter->VinsideNew;
    Vscope->VthisName = Aouter->VthisName;
    Vscope->VinsideShared = Aouter->VinsideShared;
    Vscope->VnoGenerate = Aouter->VnoGenerate;
  }
  Vscope->Vinit = 1;
  Vscope->VforwardDeclare = AforwardDeclare;
  return Vscope;
}
void Iscope() {
 static int done = 0;
 if (!done) {
  done = 1;
  Ibuiltin();
  Igenerate();
  Ioutput();
  Isymbol();
  Itokenize();
  Iusedfile();
  Izimbufile();
  Ilibarg();
  Ilibio();
  Ilibsys();
  Ilibthread();
  CScope__X__VpredefinedSymbols = NULL;
 }
}
