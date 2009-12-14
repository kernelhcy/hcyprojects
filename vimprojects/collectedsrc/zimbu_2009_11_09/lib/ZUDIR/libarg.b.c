#define INC_libarg_B 1
/*
 * FUNCTION BODIES
 */
#ifndef INC_generate_B
#include "../../ZUDIR/generate.b.c"
#endif
#ifndef INC_liststuff_B
#include "../../ZUDIR/liststuff.b.c"
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
CSymbol *MLibARG__FgetSymbol() {
  MLibARG__VmoduleSymbol = CSymbol__FNEW(20);
  MLibARG__VmoduleSymbol->Vname = "ARG";
  MLibARG__VmoduleSymbol->VcName = "MARG";
  ZListAdd(CWrite_C__X__VdeclWriters, -1, 0, MLibARG__FwriteDecl, 1);
  ZListAdd(CWrite_C__X__VbodyWriters, -1, 0, MLibARG__FwriteBodies, 1);
  CSymbol__FaddMember(MLibARG__VmoduleSymbol, "name", CSymbol__X__Vstring, 0)->VcName = "MARG__Vname";
  CSymbol__FaddMember(MLibARG__VmoduleSymbol, "count", CSymbol__X__Vint, 0)->VcName = "MARG__Vcount";
  CSymbol *Vmember;
  Vmember = NULL;
  Vmember = CSymbol__FaddMember(MLibARG__VmoduleSymbol, "args", CSymbol__FNEW(16), 0);
  Vmember->VcName = "MARG__Vargs";
  Vmember->VreturnSymbol = CSymbol__FNEW(9);
  CSymbol__FaddLibMethod(MLibARG__VmoduleSymbol, "SIZE", MLibARG__Fsize, CSymbol__X__Vint);
  Vmember = CSymbol__FaddLibMethod(MLibARG__VmoduleSymbol, "get", MLibARG__Fget, CSymbol__X__Vstring);
  CSymbol__FaddMember(Vmember, "index", CSymbol__X__Vint, 0);
  CSymbol *Vret;
  Vret = CSymbol__FNEW(13);
  Vret->VreturnSymbol = CSymbol__X__Vstring;
  CSymbol__FaddLibMethod(MLibARG__VmoduleSymbol, "getAll", MLibARG__FgetAll, Vret);
  Vret = CSymbol__FNEW(13);
  Vret->VreturnSymbol = CSymbol__X__Vstring;
  CSymbol__FaddLibMethod(MLibARG__VmoduleSymbol, "getClean", MLibARG__FgetClean, Vret);
  return MLibARG__VmoduleSymbol;
}
void MLibARG__Fsize(CSymbol *Asym, CSymbol *Aclass, CNode *Aarg_node, CSContext *Actx) {
  MLibARG__VuseCount = 1;
  COutput__Fwrite(Actx->Vout, "MARG__Vcount");
}
void MLibARG__Fget(CSymbol *Asym, CSymbol *Aclass, CNode *Aarg_node, CSContext *Actx) {
  MLibARG__VuseCount = 1;
  MLibARG__VuseArgs = 1;
  MLibARG__VuseZArgGet = 1;
  COutput__Fwrite(Actx->Vout, "ZArgGet(");
  MGenerate__FgenExpr(Aarg_node, Actx, CSymbol__X__Vint);
  COutput__Fwrite(Actx->Vout, ")");
}
void MLibARG__FgetAll(CSymbol *Asym, CSymbol *Aclass, CNode *Aarg_node, CSContext *Actx) {
  MLibARG__VuseCount = 1;
  MLibARG__VuseArgs = 1;
  MLibARG__VuseZArgAll = 1;
  CScope__FaddUsedItem(Actx->Vscope, "alloc");
  CScope__FaddUsedItem(Actx->Vscope, "listAdd");
  COutput__Fwrite(Actx->Vout, "ZArgAll()");
}
void MLibARG__FgetClean(CSymbol *Asym, CSymbol *Aclss, CNode *Aarg_node, CSContext *Actx) {
  MLibARG__VuseCount = 1;
  MLibARG__VuseArgs = 1;
  MLibARG__VuseZArgClean = 1;
  CScope__FaddUsedItem(Actx->Vscope, "alloc");
  CScope__FaddUsedItem(Actx->Vscope, "listAdd");
  COutput__Fwrite(Actx->Vout, "ZArgClean()");
}
void MLibARG__FcheckUsed() {
  if ((MLibARG__VmoduleSymbol != NULL))
  {
    {
      Zfor_T *Zf = ZforNew(MLibARG__VmoduleSymbol->VmemberList, 2);
      CSymbol *Vsym;
      for (ZforGetPtr(Zf, (char **)&Vsym); ZforCont(Zf); ZforNextPtr(Zf, (char **)&Vsym)) {
        if (((Zstrcmp(Vsym->Vname, "count") == 0) && Vsym->Vused))
        {
          MLibARG__VuseCount = 1;
        }
      else if (((Zstrcmp(Vsym->Vname, "name") == 0) && Vsym->Vused))
        {
          MLibARG__VuseName = 1;
        }
      else if (((Zstrcmp(Vsym->Vname, "args") == 0) && Vsym->Vused))
        {
          MLibARG__VuseArgs = 1;
        }
      }
    }
  }
}
void MLibARG__FwriteDecl(CScope *AtopScope, FILE *Afd) {
  MLibARG__FcheckUsed();
  if (MLibARG__VuseCount)
  {
    fputs("\nZint MARG__Vcount;\n", Afd);
  }
  if (MLibARG__VuseName)
  {
    fputs("\nchar *MARG__Vname;\n", Afd);
  }
  if (MLibARG__VuseArgs)
  {
    fputs("\nchar **MARG__Vargs;\n", Afd);
  }
}
void MLibARG__FwriteBodies(CScope *AtopScope, FILE *Afd) {
  if (MLibARG__VuseZArgGet)
  {
    fputs("\nchar *ZArgGet(Zint i) {\n  if (i < 0 || i >= MARG__Vcount)\n    return NULL;\n  return MARG__Vargs[i];\n}\n", Afd);
  }
  if (MLibARG__VuseZArgAll)
  {
    fputs("\nCListHead *ZArgAll() {\n  int i;\n  CListHead *head = Zalloc(sizeof(CListHead));\n  for (i = 0; i < MARG__Vcount; ++i) {\n    ZListAdd(head, -1, 0, MARG__Vargs[i], 1);\n  }\n  return head;\n}\n", Afd);
  }
  if (MLibARG__VuseZArgClean)
  {
    fputs("\nCListHead *ZArgClean() {\n  int i;\n  CListHead *head = Zalloc(sizeof(CListHead));\n  for (i = 0; i < MARG__Vcount; ++i) {\n    ZListAdd(head, -1, 0, MARG__Vargs[i], 1);\n  }\n  return head;\n}\n", Afd);
  }
}
void MLibARG__FwriteMain(COutput *Aout) {
  MLibARG__FcheckUsed();
  if (MLibARG__VuseCount)
  {
    COutput__Fwrite(Aout, "  MARG__Vcount = argc - 1;\n");
  }
  if (MLibARG__VuseName)
  {
    COutput__Fwrite(Aout, "  MARG__Vname = argv[0];\n");
  }
  if (MLibARG__VuseArgs)
  {
    COutput__Fwrite(Aout, "  MARG__Vargs = argv + 1;\n");
  }
}
void Ilibarg() {
 static int done = 0;
 if (!done) {
  done = 1;
  Igenerate();
  Iliststuff();
  Ioutput();
  Iscontext();
  Iscope();
  Isymbol();
  Iwrite_c();
  MLibARG__VuseCount = 0;
  MLibARG__VuseName = 0;
  MLibARG__VuseArgs = 0;
  MLibARG__VuseZArgGet = 0;
  MLibARG__VuseZArgAll = 0;
  MLibARG__VuseZArgClean = 0;
  MLibARG__VmoduleSymbol = NULL;
 }
}
