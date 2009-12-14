#define INC_usedfile_S 1
/*
 * STRUCTS
 */
#ifndef INC_builtin_S
#include "../ZUDIR/builtin.s.h"
#endif
#ifndef INC_scope_S
#include "../ZUDIR/scope.s.h"
#endif
#ifndef INC_zimbufile_S
#include "../ZUDIR/zimbufile.s.h"
#endif
struct CUsedFile__S {
  Zbool VisMainFile;
  Zbool VisTopFile;
  Zbool VzwtFile;
  CUsedFile *Vparent;
  CZimbuFile *VzimbuFile;
  CDictHead *VusedImports;
  CDictHead *VusedBuiltins;
};
