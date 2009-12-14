#define INC_libsys_B 1
/*
 * FUNCTION BODIES
 */
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
CSymbol *MLibSYS__FgetSymbol() {
  CSymbol *Vsym;
  Vsym = CSymbol__FNEW(20);
  Vsym->Vname = "SYS";
  Vsym->VcName = "MSYS";
  ZListAdd(CWrite_C__X__VbodyWriters, -1, 0, MLibSYS__FwriteBody, 1);
  CSymbol *Vmember;
  Vmember = NULL;
  Vmember = CSymbol__FaddLibMethod(Vsym, "shell", MLibSYS__Fshell, CSymbol__X__Vint);
  CSymbol__FaddMember(Vmember, "command", CSymbol__X__Vstring, 0);
  Vmember = CSymbol__FaddLibMethod(Vsym, "sleep", MLibSYS__Fsleep, NULL);
  CSymbol__FaddMember(Vmember, "usec", CSymbol__X__Vint, 0);
  Vmember = CSymbol__FaddLibMethod(Vsym, "sleepSec", MLibSYS__FsleepSec, NULL);
  CSymbol__FaddMember(Vmember, "sec", CSymbol__X__Vint, 0);
  Vmember = CSymbol__FaddLibMethod(Vsym, "malloc", MLibSYS__Fmalloc, CSymbol__X__Vstring);
  CSymbol__FaddMember(Vmember, "size", CSymbol__X__Vint, 0);
  Vmember = CSymbol__FaddLibMethod(Vsym, "realloc", MLibSYS__Frealloc, CSymbol__X__Vstring);
  CSymbol__FaddMember(Vmember, "buf", CSymbol__X__Vstring, 0);
  CSymbol__FaddMember(Vmember, "size", CSymbol__X__Vint, 0);
  return Vsym;
}
void MLibSYS__Fshell(CSymbol *Asym, CSymbol *Aclass, CNode *Aarg_node, CSContext *Actx) {
  COutput__Fwrite(Actx->Vout, "fflush(stdout), ");
  COutput__Fwrite(Actx->Vout, "system(");
  MLibSYS__FgenExpr(Aarg_node, Actx, CSymbol__X__Vstring);
  COutput__Fwrite(Actx->Vout, ")");
}
void MLibSYS__Fsleep(CSymbol *Asym, CSymbol *Aclass, CNode *Aarg_node, CSContext *Actx) {
  MLibSYS__VuseZsleep = 1;
  CScope__FaddUsedItem(Actx->Vscope, "time.h");
  COutput__Fwrite(Actx->Vout, "Zsleep(");
  MLibSYS__FgenExpr(Aarg_node, Actx, CSymbol__X__Vint);
  COutput__Fwrite(Actx->Vout, ")");
}
void MLibSYS__FsleepSec(CSymbol *Asym, CSymbol *Aclass, CNode *Aarg_node, CSContext *Actx) {
  MLibSYS__VuseZsleep = 1;
  CScope__FaddUsedItem(Actx->Vscope, "time.h");
  COutput__Fwrite(Actx->Vout, "Zsleep((");
  MLibSYS__FgenExpr(Aarg_node, Actx, CSymbol__X__Vint);
  COutput__Fwrite(Actx->Vout, ") * 1000000LL)");
}
void MLibSYS__Fmalloc(CSymbol *Asym, CSymbol *Aclass, CNode *Aarg_node, CSContext *Actx) {
  COutput__Fwrite(Actx->Vout, "malloc(");
  MLibSYS__FgenExpr(Aarg_node, Actx, CSymbol__X__Vint);
  COutput__Fwrite(Actx->Vout, ")");
}
void MLibSYS__Frealloc(CSymbol *Asym, CSymbol *Aclass, CNode *Aarg_node, CSContext *Actx) {
  COutput__Fwrite(Actx->Vout, "realloc(");
  if ((Aarg_node->Vn_type != 110))
  {
    CNode__Ferror(Aarg_node, "expected more arguments to realloc()");
  }
else
  {
    MLibSYS__FgenExpr(Aarg_node->Vn_left, Actx, CSymbol__X__Vstring);
    COutput__Fwrite(Actx->Vout, ", ");
    MLibSYS__FgenExpr(Aarg_node->Vn_right, Actx, CSymbol__X__Vint);
    COutput__Fwrite(Actx->Vout, ")");
    Aarg_node->Vn_undefined = (Aarg_node->Vn_left->Vn_undefined + Aarg_node->Vn_right->Vn_undefined);
  }
}
CSymbol *MLibSYS__FgenExpr(CNode *Anode, CSContext *Actx, CSymbol *AdestSym) {
  return MGenerate__FgenExpr(Anode, Actx, AdestSym);
}
void MLibSYS__FwriteBody(CScope *AtopScope, FILE *Afd) {
  if (MLibSYS__VuseZsleep)
  {
    fputs("\nvoid Zsleep(Zint usec) {\n  struct timespec tm;\n  tm.tv_sec = usec / 1000000;\n  tm.tv_nsec = (usec % 1000000) * 1000;\n  nanosleep(&tm, NULL);\n}\n", Afd);
  }
}
void Ilibsys() {
 static int done = 0;
 if (!done) {
  done = 1;
  Igenerate();
  Ioutput();
  Iscontext();
  Iscope();
  Isymbol();
  Iwrite_c();
  MLibSYS__VuseZsleep = 0;
 }
}
