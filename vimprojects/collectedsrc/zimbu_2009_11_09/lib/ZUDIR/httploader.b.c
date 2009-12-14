#define INC_httploader_B 1
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
#ifndef INC_write_c_B
#include "../../ZUDIR/write_c.b.c"
#endif
CBuiltin *MHTTPloader__Fprepare() {
  CBuiltin *Vb;
  Vb = Zalloc(sizeof(CBuiltin));
  Vb->VmoduleName = "HTTPmodule";
  Vb->VfileName = "httpmodule.zu";
  Vb->Vsetup = MHTTPloader__Fsetup;
  return Vb;
}
void MHTTPloader__Fsetup(CBuiltin *Abuiltin) {
  MHTTPloader__VuseServer = 1;
  if ((MHTTPloader__VmoduleSymbol == NULL))
  {
    MHTTPloader__VmoduleSymbol = CSymbol__FNEW(20);
    MHTTPloader__VmoduleSymbol->Vname = "HTTP";
    MHTTPloader__VmoduleSymbol->VcName = "MHTTP";
    CScope__X__FaddPredefinedSymbol(MHTTPloader__VmoduleSymbol);
    CBuiltin__FaddAllMembers(Abuiltin, MHTTPloader__VmoduleSymbol);
    ZListAdd(CWrite_C__X__VincludeWriters, -1, 0, MHTTPloader__FwriteIncl, 1);
  }
}
void MHTTPloader__FwriteIncl(CScope *AtopScope, FILE *Afd) {
  if (MHTTPloader__VuseServer)
  {
    fputs("\n#include <sys/types.h>\n#include <sys/socket.h>\n#include <netinet/in.h>\n#include <arpa/inet.h>\n", Afd);
  }
}
void Ihttploader() {
 static int done = 0;
 if (!done) {
  done = 1;
  Ibuiltin();
  Inode();
  Iscope();
  Isymbol();
  Iwrite_c();
  MHTTPloader__VuseServer = 0;
  MHTTPloader__VmoduleSymbol = NULL;
 }
}
