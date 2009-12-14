#define INC_zimbufile_S 1
/*
 * STRUCTS
 */
#ifndef INC_error_S
#include "../ZUDIR/error.s.h"
#endif
#ifndef INC_node_S
#include "../ZUDIR/node.s.h"
#endif
#ifndef INC_output_S
#include "../ZUDIR/output.s.h"
#endif
#ifndef INC_parse_S
#include "../ZUDIR/parse.s.h"
#endif
#ifndef INC_scope_S
#include "../ZUDIR/scope.s.h"
#endif
#ifndef INC_symbol_S
#include "../ZUDIR/symbol.s.h"
#endif
#ifndef INC_usedfile_S
#include "../ZUDIR/usedfile.s.h"
#endif
struct CZimbuFile__X__CCodeSpecific__S {
  char *VstartedWrite;
  COutput__X__CHeads *Vheads;
  COutput__X__CGroup *Voutputs;
};
struct CZimbuFile__S {
  char *Vfilename;
  char *VrootName;
  char *VdirName;
  char *VoutDir;
  char *VinitFunc;
  Zint VstartedPass;
  Zbool VdidInitFunc;
  Zbool VtopZwtFile;
  Zbool VusedAsZwt;
  Zbool VusedAsZimbu;
  CScope *VtopScope;
  CZimbuFile__X__CCodeSpecific *Vc;
  CZimbuFile__X__CCodeSpecific *Vjs;
};
