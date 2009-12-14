#define INC_zwtloader_B 1
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
CBuiltin *MZWTloader__Fprepare() {
  CBuiltin *Vb;
  Vb = Zalloc(sizeof(CBuiltin));
  Vb->VmoduleName = "ZWTmodule";
  Vb->VfileName = "zwtmodule.zu";
  Vb->Vsetup = MZWTloader__Fsetup;
  return Vb;
}
void MZWTloader__Fsetup(CBuiltin *Abuiltin) {
  MZWTloader__VuseServer = 1;
  if ((MZWTloader__VmoduleSymbol == NULL))
  {
    MZWTloader__VmoduleSymbol = CSymbol__FNEW(20);
    MZWTloader__VmoduleSymbol->Vname = "ZWT";
    MZWTloader__VmoduleSymbol->VcName = "MZWT";
    CScope__X__FaddPredefinedSymbol(MZWTloader__VmoduleSymbol);
    CBuiltin__FaddAllMembers(Abuiltin, MZWTloader__VmoduleSymbol);
  }
}
void Izwtloader() {
 static int done = 0;
 if (!done) {
  done = 1;
  Ibuiltin();
  Inode();
  Iscope();
  Isymbol();
  MZWTloader__VuseServer = 0;
  MZWTloader__VmoduleSymbol = NULL;
 }
}
