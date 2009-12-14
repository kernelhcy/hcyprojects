#define INC_scontext_B 1
/*
 * FUNCTION BODIES
 */
#ifndef INC_output_B
#include "../ZUDIR/output.b.c"
#endif
#ifndef INC_resolve_B
#include "../ZUDIR/resolve.b.c"
#endif
#ifndef INC_scope_B
#include "../ZUDIR/scope.b.c"
#endif
CSContext *CSContext__FNEW(CScope *A_scope, Zoref *A_gen, COutput *A_out) {
  CSContext *THIS = Zalloc(sizeof(CSContext));
  THIS->Vscope = A_scope;
  THIS->Vgen = A_gen;
  THIS->Vout = A_out;
  return THIS;
}
CSContext *CSContext__Fcopy(CSContext *THIS) {
  return CSContext__FNEW(THIS->Vscope, THIS->Vgen, THIS->Vout);
}
CSContext *CSContext__FcopyNewOut(CSContext *THIS) {
  CSContext *Vr;
  Vr = CSContext__FNEW(THIS->Vscope, THIS->Vgen, COutput__FNEW(COutput__X__CFragmentHead__FNEW__1()));
  Vr->Vout->Vwriting = THIS->Vout->Vwriting;
  return Vr;
}
CSContext *CSContext__FcopyNoOut(CSContext *THIS) {
  CSContext *Vr;
  Vr = CSContext__FNEW(THIS->Vscope, THIS->Vgen, COutput__X__VnoOut);
  return Vr;
}
void Iscontext() {
 static int done = 0;
 if (!done) {
  done = 1;
  Ioutput();
  Iresolve();
  Iscope();
 }
}
