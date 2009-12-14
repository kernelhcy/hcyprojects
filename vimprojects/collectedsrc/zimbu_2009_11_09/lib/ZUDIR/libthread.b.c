#define INC_libthread_B 1
/*
 * FUNCTION BODIES
 */
#ifndef INC_attr_B
#include "../../ZUDIR/attr.b.c"
#endif
#ifndef INC_config_B
#include "../../ZUDIR/config.b.c"
#endif
#ifndef INC_generate_B
#include "../../ZUDIR/generate.b.c"
#endif
#ifndef INC_node_B
#include "../../ZUDIR/node.b.c"
#endif
#ifndef INC_output_B
#include "../../ZUDIR/output.b.c"
#endif
#ifndef INC_scontext_B
#include "../../ZUDIR/scontext.b.c"
#endif
#ifndef INC_scope_B
#include "../../ZUDIR/scope.b.c"
#endif
#ifndef INC_symbol_B
#include "../../ZUDIR/symbol.b.c"
#endif
#ifndef INC_write_c_B
#include "../../ZUDIR/write_c.b.c"
#endif
CListHead *MLibTHREAD__FgetSymbols() {
  MLibTHREAD__VmoduleSym = CSymbol__FNEW(20);
  MLibTHREAD__VmoduleSym->Vname = "THREAD";
  MLibTHREAD__VmoduleSym->VcName = "MTHREAD";
  ZListAdd(CWrite_C__X__VincludeWriters, -1, 0, MLibTHREAD__FwriteIncludes, 1);
  ZListAdd(CWrite_C__X__VtypedefWriters, -1, 0, MLibTHREAD__FwriteTypedefs, 1);
  ZListAdd(CWrite_C__X__VdeclWriters, -1, 0, MLibTHREAD__FwriteDecl, 1);
  ZListAdd(CWrite_C__X__VbodyWriters, -1, 0, MLibTHREAD__FwriteBody, 1);
  CSymbol *VstateSymbol;
  VstateSymbol = CSymbol__FaddMember(MLibTHREAD__VmoduleSym, "State", CSymbol__FNEW(29), 0);
  VstateSymbol->Vclass = VstateSymbol;
  VstateSymbol->VcName = "MTHREAD_EState";
  CSymbol__FaddMember(VstateSymbol, "new", CSymbol__FNEW(30), 0);
  CSymbol__FaddMember(VstateSymbol, "running", CSymbol__FNEW(30), 1);
  CSymbol__FaddMember(VstateSymbol, "finished", CSymbol__FNEW(30), 2);
  CSymbol *VtypeSymbol;
  VtypeSymbol = CSymbol__FaddMember(MLibTHREAD__VmoduleSym, "Type", CSymbol__FNEW(29), 0);
  VtypeSymbol->Vclass = VtypeSymbol;
  VtypeSymbol->VcName = "MTHREAD_EType";
  CSymbol__FaddMember(VtypeSymbol, "main", CSymbol__FNEW(30), 0);
  CSymbol__FaddMember(VtypeSymbol, "daemon", CSymbol__FNEW(30), 1);
  CSymbol__FaddMember(VtypeSymbol, "normal", CSymbol__FNEW(30), 2);
  MLibTHREAD__VthreadSym = CSymbol__FNEW(21);
  MLibTHREAD__VthreadSym->Vname = "thread";
  MLibTHREAD__VthreadSym->Vclass = MLibTHREAD__VthreadSym;
  MLibTHREAD__VthreadSym->VcName = "Zthread";
  MLibTHREAD__VthreadSym->VclassName = "Zthread";
  CSymbol__FaddMember(MLibTHREAD__VthreadSym, "name", CSymbol__X__Vstring, 0)->VcName = "name";
  CSymbol__FaddMember(MLibTHREAD__VthreadSym, "state", VstateSymbol, 0)->VcName = "state";
  CSymbol__FaddMember(MLibTHREAD__VthreadSym, "type", VtypeSymbol, 0)->VcName = "type";
  CSymbol__FaddMember(MLibTHREAD__VthreadSym, "keep", CSymbol__X__Vbool, 0)->VcName = "keep";
  MLibTHREAD__VthreadSym->VstructOut = COutput__FNEW(COutput__X__CFragmentHead__FNEW__1());
  MLibTHREAD__VthreadSym->VstructOut->Vwriting = 1;
  COutput__Fwrite(MLibTHREAD__VthreadSym->VstructOut, "  char *name;\n");
  COutput__Fwrite(MLibTHREAD__VthreadSym->VstructOut, "  int state;\n");
  COutput__Fwrite(MLibTHREAD__VthreadSym->VstructOut, "  int type;\n");
  COutput__Fwrite(MLibTHREAD__VthreadSym->VstructOut, "  int keep;\n");
  COutput__Fwrite(MLibTHREAD__VthreadSym->VstructOut, "  void (*proc)();\n");
  COutput__Fwrite(MLibTHREAD__VthreadSym->VstructOut, "  pthread_t id;\n");
  CSymbol *Vmember;
  Vmember = NULL;
  CSymbol *VfuncSym;
  VfuncSym = NULL;
  VfuncSym = CSymbol__FaddLibMethod(MLibTHREAD__VthreadSym, "NEW", MLibTHREAD__FthreadNew, NULL);
  VfuncSym = CSymbol__FaddLibMethod(MLibTHREAD__VthreadSym, "NEW", MLibTHREAD__FthreadNewProc, NULL);
  CSymbol__FaddMember(VfuncSym, "proc", CSymbol__X__Vproc_ref, 0);
  VfuncSym = CSymbol__FaddLibMethod(MLibTHREAD__VthreadSym, "body", MLibTHREAD__FthreadBody, NULL);
  VfuncSym->Vattributes = 2;
  VfuncSym = CSymbol__FaddLibMethod(MLibTHREAD__VthreadSym, "start", MLibTHREAD__FthreadStart, NULL);
  VfuncSym = CSymbol__FaddLibMethod(MLibTHREAD__VthreadSym, "setProc", MLibTHREAD__FthreadSetProc, NULL);
  CSymbol__FaddMember(VfuncSym, "proc", CSymbol__X__Vproc_ref, 0);
  CSymbol__FaddLibMethod(MLibTHREAD__VthreadSym, "wait", MLibTHREAD__FthreadWait, NULL);
  CSymbol__FaddLibMethod(MLibTHREAD__VthreadSym, "kill", MLibTHREAD__FthreadKill, NULL);
  CSymbol__FaddLibMethod(MLibTHREAD__VthreadSym, "remove", MLibTHREAD__FthreadRemove, NULL);
  CSymbol__FaddMember__1(MLibTHREAD__VmoduleSym, MLibTHREAD__VthreadSym);
  VfuncSym = CSymbol__FaddLibMethod(MLibTHREAD__VmoduleSym, "threads", MLibTHREAD__Fthreads, NULL);
  Vmember = CSymbol__FNEW(13);
  Vmember->VreturnSymbol = MLibTHREAD__VthreadSym;
  VfuncSym->VreturnSymbol = Vmember;
  VfuncSym = CSymbol__FaddLibMethod(MLibTHREAD__VmoduleSym, "current", MLibTHREAD__Fcurrent, NULL);
  VfuncSym->VreturnSymbol = MLibTHREAD__VthreadSym;
  return ZListAdd(ZListAdd(Zalloc(sizeof(CListHead)), -1, 0, MLibTHREAD__VmoduleSym, 1), -1, 0, MLibTHREAD__VthreadSym, 1);
}
void MLibTHREAD__Fthreads(CSymbol *Asym, CSymbol *Aclass, CNode *Am_node, CSContext *Actx) {
  CNode__Ferror(Am_node, "THREAD.threads() not implemented yet");
}
void MLibTHREAD__Fcurrent(CSymbol *Asym, CSymbol *Aclass, CNode *Am_node, CSContext *Actx) {
  CNode__Ferror(Am_node, "THREAD.current() not implemented yet");
}
void MLibTHREAD__FthreadNew(CSymbol *Asym, CSymbol *Aclass, CNode *Am_node, CSContext *Actx) {
  char *Vname;
  Vname = MLibTHREAD__FparentClassName(Asym, MLibTHREAD__VthreadNewClasses);
  COutput__Fwrite(Actx->Vout, Zconcat(Vname, "ZThreadNew()"));
}
void MLibTHREAD__FthreadNewProc(CSymbol *Asym, CSymbol *Aclass, CNode *Am_node, CSContext *Actx) {
  char *Vname;
  Vname = MLibTHREAD__FparentClassName(Asym, MLibTHREAD__VthreadNewProcClasses);
  COutput__Fwrite(Actx->Vout, Zconcat(Vname, "ZThreadNewProc("));
  if ((Am_node->Vn_right == NULL))
  {
    CNode__Ferror(Am_node, "missing proc argument");
  }
else
  {
    MGenerate__FgenExpr(Am_node->Vn_right, Actx, CSymbol__X__Vproc_ref);
    Am_node->Vn_undefined = Am_node->Vn_right->Vn_undefined;
  }
  COutput__Fwrite(Actx->Vout, ")");
}
void MLibTHREAD__FthreadBody(CSymbol *Asym, CSymbol *Aclass, CNode *Am_node, CSContext *Actx) {
  char *Vname;
  Vname = MLibTHREAD__FparentClassName(Asym, MLibTHREAD__VthreadBodyClasses);
  COutput__Fwrite(Actx->Vout, Zconcat(Vname, "ZThreadBody("));
  MLibTHREAD__FgenerateVarname(Am_node->Vn_left->Vn_left, Actx, MLibTHREAD__VthreadSym);
  COutput__Fwrite(Actx->Vout, ")");
  Am_node->Vn_undefined = Am_node->Vn_left->Vn_left->Vn_undefined;
}
void MLibTHREAD__FthreadStart(CSymbol *Asym, CSymbol *Aclass, CNode *Am_node, CSContext *Actx) {
  char *Vname;
  Vname = MLibTHREAD__FparentClassName(Asym, MLibTHREAD__VthreadStartClasses);
  COutput__Fwrite(Actx->Vout, Zconcat(Vname, "ZThreadStart("));
  MLibTHREAD__FgenerateVarname(Am_node->Vn_left->Vn_left, Actx, MLibTHREAD__VthreadSym);
  Am_node->Vn_undefined = Am_node->Vn_left->Vn_left->Vn_undefined;
  CSymbol *Vs;
  Vs = CSymbol__FfindMember(Aclass, "body");
  if (((Vs == NULL) || (Vs->VcName == NULL)))
  {
    COutput__Fwrite(Actx->Vout, Zconcat(Zconcat(", ", Vname), "ZThreadBody)"));
  }
else
  {
    COutput__Fwrite(Actx->Vout, Zconcat(Zconcat(", ", Vs->VcName), ")"));
  }
  MLibTHREAD__FparentClassName(Asym, MLibTHREAD__VthreadBodyClasses);
}
void MLibTHREAD__FthreadSetProc(CSymbol *Asym, CSymbol *Aclass, CNode *Am_node, CSContext *Actx) {
  char *Vname;
  Vname = MLibTHREAD__FparentClassName(Asym, MLibTHREAD__VthreadSetprocClasses);
  COutput__Fwrite(Actx->Vout, Zconcat(Vname, "ZThreadSetProc("));
  MLibTHREAD__FgenerateVarname(Am_node->Vn_left->Vn_left, Actx, MLibTHREAD__VthreadSym);
  COutput__Fwrite(Actx->Vout, ", ");
  MGenerate__FgenExpr(Am_node->Vn_right, Actx, CSymbol__X__Vproc_ref);
  COutput__Fwrite(Actx->Vout, ")");
  Am_node->Vn_undefined = (Am_node->Vn_left->Vn_left->Vn_undefined + Am_node->Vn_right->Vn_undefined);
}
void MLibTHREAD__FthreadWait(CSymbol *Asym, CSymbol *Aclass, CNode *Am_node, CSContext *Actx) {
  char *Vname;
  Vname = MLibTHREAD__FparentClassName(Asym, MLibTHREAD__VthreadWaitClasses);
  COutput__Fwrite(Actx->Vout, Zconcat(Vname, "ZThreadWait("));
  MLibTHREAD__FgenerateVarname(Am_node->Vn_left->Vn_left, Actx, MLibTHREAD__VthreadSym);
  COutput__Fwrite(Actx->Vout, ")");
  Am_node->Vn_undefined = Am_node->Vn_left->Vn_left->Vn_undefined;
}
void MLibTHREAD__FthreadKill(CSymbol *Asym, CSymbol *Aclass, CNode *Am_node, CSContext *Actx) {
  char *Vname;
  Vname = MLibTHREAD__FparentClassName(Asym, MLibTHREAD__VthreadKillClasses);
  COutput__Fwrite(Actx->Vout, Zconcat(Vname, "ZThreadKill("));
  MLibTHREAD__FgenerateVarname(Am_node->Vn_left->Vn_left, Actx, ((Asym->Vclass == NULL)) ? (MLibTHREAD__VthreadSym) : (Asym->Vclass));
  COutput__Fwrite(Actx->Vout, ")");
  Am_node->Vn_undefined = Am_node->Vn_left->Vn_left->Vn_undefined;
}
CSymbol *MLibTHREAD__FgenerateVarname(CNode *Anode, CSContext *Actx, CSymbol *AdestSym) {
  return MGenerate__FgenerateVarname(Anode, Actx, AdestSym);
}
void MLibTHREAD__FthreadRemove(CSymbol *Asym, CSymbol *Aclass, CNode *Am_node, CSContext *Actx) {
  CNode__Ferror(Am_node, "thread.remove() not implemented yet");
}
char *MLibTHREAD__FparentClassName(CSymbol *Asym, CDictHead *Aclasses) {
  char *Vname;
  Vname = NULL;
  if ((Asym->Vclass == NULL))
  {
    Vname = "";
  }
else
  {
    Vname = Asym->Vclass->VclassName;
  }
  if (!(ZDictHas(Aclasses, 0, Vname)))
  {
    ZDictAdd(1, Aclasses, 0, Vname, 0, Asym->Vclass);
  }
  if ((Zstrcmp(Vname, "") == 0))
  {
    return "";
  }
  return Zconcat(Vname, "__");
}
void MLibTHREAD__FcheckUsed() {
  if ((MLibTHREAD__VmoduleSym != NULL))
  {
    if ((((((MLibTHREAD__VthreadBodyClasses->used > 0) || (MLibTHREAD__VthreadStartClasses->used > 0)) || (MLibTHREAD__VthreadSetprocClasses->used > 0)) || (MLibTHREAD__VthreadWaitClasses->used > 0)) || (MLibTHREAD__VthreadKillClasses->used > 0)))
    {
      MLibTHREAD__VuseThread = 1;
    }
  }
}
void MLibTHREAD__FwriteIncludes(CScope *AtopScope, FILE *Afd) {
  MLibTHREAD__FcheckUsed();
  if (MLibTHREAD__VuseThread)
  {
    MConfig__FaddThreadLib();
    fputs("\n#include <pthread.h>\n", Afd);
  }
}
void MLibTHREAD__FwriteTypedefs(CScope *AtopScope, FILE *Afd) {
  if (MLibTHREAD__VuseThread)
  {
    fputs("\ntypedef struct Zthread__S Zthread;\n", Afd);
  }
}
void MLibTHREAD__FwriteDecl(CScope *AtopScope, FILE *Afd) {
  if (MLibTHREAD__VuseThread)
  {
    fputs("\nstruct Zthread__S {\n", Afd);
    fputs(COutput__FtoString(MLibTHREAD__VthreadSym->VstructOut), Afd);
    fputs("};\n", Afd);
  }
}
void MLibTHREAD__FwriteFuncLead(FILE *Afd, char *Aname, char *Afuncname) {
  fputs("\nvoid ", Afd);
  if ((Zstrcmp(Aname, "") != 0))
  {
    fputs(Zconcat(Aname, "__"), Afd);
  }
  fputs(Zconcat(Afuncname, "("), Afd);
  if ((Zstrcmp(Aname, "") == 0))
  {
    fputs("Zthread", Afd);
  }
else
  {
    fputs(Aname, Afd);
  }
  fputs(" *t", Afd);
}
void MLibTHREAD__FwriteBody(CScope *AtopScope, FILE *Afd) {
  {
    Zfor_T *Zf = ZforNew(ZDictKeys(MLibTHREAD__VthreadNewClasses), 2);
    char *Vname;
    for (ZforGetPtr(Zf, (char **)&Vname); ZforCont(Zf); ZforNextPtr(Zf, (char **)&Vname)) {
      char *VtypeName;
      VtypeName = NULL;
      char *Vextra;
      Vextra = NULL;
      if ((Zstrcmp(Vname, "") == 0))
      {
        VtypeName = "Zthread";
        Vextra = "";
      }
    else
      {
        VtypeName = Vname;
        Vextra = "__";
      }
      fputs(Zconcat(Zconcat(Zconcat(Zconcat(Zconcat("\n", VtypeName), " *"), Vname), Vextra), "ZThreadNew() {\n"), Afd);
      fputs(Zconcat(Zconcat("  return Zalloc(sizeof(", VtypeName), "));\n"), Afd);
      fputs("}\n", Afd);
    }
  }
  {
    Zfor_T *Zf = ZforNew(ZDictKeys(MLibTHREAD__VthreadNewProcClasses), 2);
    char *Vname;
    for (ZforGetPtr(Zf, (char **)&Vname); ZforCont(Zf); ZforNextPtr(Zf, (char **)&Vname)) {
      char *VtypeName;
      VtypeName = NULL;
      char *Vextra;
      Vextra = NULL;
      if ((Zstrcmp(Vname, "") == 0))
      {
        VtypeName = "Zthread";
        Vextra = "";
      }
    else
      {
        VtypeName = Vname;
        Vextra = "__";
      }
      fputs(Zconcat(Zconcat(Zconcat(Zconcat(Zconcat("\n", VtypeName), " *"), Vname), Vextra), "ZThreadNewProc("), Afd);
      fputs("void (*proc)()) {\n", Afd);
      fputs(Zconcat(Zconcat(Zconcat(Zconcat("  ", VtypeName), " *t = Zalloc(sizeof("), VtypeName), "));\n"), Afd);
      fputs("  t->proc = proc;\n", Afd);
      fputs("  return t;\n", Afd);
      fputs("}\n", Afd);
    }
  }
  {
    Zfor_T *Zf = ZforNew(ZDictKeys(MLibTHREAD__VthreadBodyClasses), 2);
    char *Vname;
    for (ZforGetPtr(Zf, (char **)&Vname); ZforCont(Zf); ZforNextPtr(Zf, (char **)&Vname)) {
      MLibTHREAD__FwriteFuncLead(Afd, Vname, "ZThreadBody");
      fputs(") {\n  if (t->proc != NULL) {\n    t->proc();\n  }\n}\n", Afd);
    }
  }
  {
    Zfor_T *Zf = ZforNew(ZDictKeys(MLibTHREAD__VthreadSetprocClasses), 2);
    char *Vname;
    for (ZforGetPtr(Zf, (char **)&Vname); ZforCont(Zf); ZforNextPtr(Zf, (char **)&Vname)) {
      MLibTHREAD__FwriteFuncLead(Afd, Vname, "ZThreadSetProc");
      fputs(", void (*proc)()) {\n  t->proc = proc;\n}\n", Afd);
    }
  }
  {
    Zfor_T *Zf = ZforNew(ZDictKeys(MLibTHREAD__VthreadStartClasses), 2);
    char *Vname;
    for (ZforGetPtr(Zf, (char **)&Vname); ZforCont(Zf); ZforNextPtr(Zf, (char **)&Vname)) {
      MLibTHREAD__FwriteFuncLead(Afd, Vname, "ZThreadStart");
      fputs(", void (*proc)()) {\n  pthread_create(&t->id, NULL, (void *(*)(void *))proc, t);\n  t->state = 1;\n}\n", Afd);
    }
  }
  {
    Zfor_T *Zf = ZforNew(ZDictKeys(MLibTHREAD__VthreadWaitClasses), 2);
    char *Vname;
    for (ZforGetPtr(Zf, (char **)&Vname); ZforCont(Zf); ZforNextPtr(Zf, (char **)&Vname)) {
      MLibTHREAD__FwriteFuncLead(Afd, Vname, "ZThreadWait");
      fputs(") {\n  pthread_join(t->id, NULL);\n  t->state = 2;\n}\n", Afd);
    }
  }
  {
    Zfor_T *Zf = ZforNew(ZDictKeys(MLibTHREAD__VthreadKillClasses), 2);
    char *Vname;
    for (ZforGetPtr(Zf, (char **)&Vname); ZforCont(Zf); ZforNextPtr(Zf, (char **)&Vname)) {
      MLibTHREAD__FwriteFuncLead(Afd, Vname, "ZThreadKill");
      fputs(") {\n  pthread_cancel(t->id);\n}\n", Afd);
    }
  }
}
void Ilibthread() {
 static int done = 0;
 if (!done) {
  done = 1;
  Iconfig();
  Igenerate();
  Ioutput();
  Iscontext();
  Iscope();
  Isymbol();
  Iwrite_c();
  MLibTHREAD__VmoduleSym = NULL;
  MLibTHREAD__VthreadSym = NULL;
  MLibTHREAD__VuseThread = 0;
  MLibTHREAD__VthreadNewClasses = ZnewDict(1);
  MLibTHREAD__VthreadNewProcClasses = ZnewDict(1);
  MLibTHREAD__VthreadBodyClasses = ZnewDict(1);
  MLibTHREAD__VthreadStartClasses = ZnewDict(1);
  MLibTHREAD__VthreadSetprocClasses = ZnewDict(1);
  MLibTHREAD__VthreadWaitClasses = ZnewDict(1);
  MLibTHREAD__VthreadKillClasses = ZnewDict(1);
 }
}
