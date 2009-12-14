#define INC_scontext_D 1
/*
 * DECLARE FUNCTIONS AND GLOBALS
 */
#ifndef INC_output_D
#include "../ZUDIR/output.d.h"
#endif
#ifndef INC_resolve_D
#include "../ZUDIR/resolve.d.h"
#endif
#ifndef INC_scope_D
#include "../ZUDIR/scope.d.h"
#endif
CSContext *CSContext__FNEW(CScope *A_scope, Zoref *A_gen, COutput *A_out);
CSContext *CSContext__Fcopy(CSContext *THIS);
CSContext *CSContext__FcopyNewOut(CSContext *THIS);
CSContext *CSContext__FcopyNoOut(CSContext *THIS);
void Iscontext();
