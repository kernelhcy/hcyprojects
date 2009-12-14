#define INC_infoloader_B 1
/*
 * FUNCTION BODIES
 */
#ifndef INC_builtin_B
#include "../../ZUDIR/builtin.b.c"
#endif
#ifndef INC_node_B
#include "../../ZUDIR/node.b.c"
#endif
#ifndef INC_scope_B
#include "../../ZUDIR/scope.b.c"
#endif
#ifndef INC_symbol_B
#include "../../ZUDIR/symbol.b.c"
#endif
CBuiltin *MINFOloader__Fprepare() {
  CBuiltin *Vb;
  Vb = Zalloc(sizeof(CBuiltin));
  Vb->VmoduleName = "INFOmodule";
  Vb->VfileName = "infomodule.zu";
  Vb->Vsetup = MINFOloader__Fsetup;
  return Vb;
}
void MINFOloader__Fsetup(CBuiltin *Abuiltin) {
  if ((MINFOloader__VmoduleSymbol == NULL))
  {
    MINFOloader__VmoduleSymbol = CSymbol__FNEW(20);
    MINFOloader__VmoduleSymbol->Vname = "INFO";
    MINFOloader__VmoduleSymbol->VcName = "MINFO";
    CScope__X__FaddPredefinedSymbol(MINFOloader__VmoduleSymbol);
    CBuiltin__FaddAllMembers(Abuiltin, MINFOloader__VmoduleSymbol);
  }
}
void Iinfoloader() {
 static int done = 0;
 if (!done) {
  done = 1;
  Ibuiltin();
  Inode();
  Iscope();
  Isymbol();
  MINFOloader__VmoduleSymbol = NULL;
 }
}
