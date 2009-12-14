#define INC_scope_S 1
/*
 * STRUCTS
 */
#ifndef INC_builtin_S
#include "../ZUDIR/builtin.s.h"
#endif
#ifndef INC_error_S
#include "../ZUDIR/error.s.h"
#endif
#ifndef INC_generate_S
#include "../ZUDIR/generate.s.h"
#endif
#ifndef INC_node_S
#include "../ZUDIR/node.s.h"
#endif
#ifndef INC_output_S
#include "../ZUDIR/output.s.h"
#endif
#ifndef INC_symbol_S
#include "../ZUDIR/symbol.s.h"
#endif
#ifndef INC_tokenize_S
#include "../ZUDIR/tokenize.s.h"
#endif
#ifndef INC_usedfile_S
#include "../ZUDIR/usedfile.s.h"
#endif
#ifndef INC_zimbufile_S
#include "../ZUDIR/zimbufile.s.h"
#endif
#ifndef INC_libarg_S
#include "../lib/ZUDIR/libarg.s.h"
#endif
#ifndef INC_libio_S
#include "../lib/ZUDIR/libio.s.h"
#endif
#ifndef INC_libsys_S
#include "../lib/ZUDIR/libsys.s.h"
#endif
#ifndef INC_libthread_S
#include "../lib/ZUDIR/libthread.s.h"
#endif
struct CScope__S {
  CScope *Vouter;
  CZimbuFile *VzimbuFile;
  char *VdirName;
  Zint Vpass;
  Zint Vdepth;
  char *VimportIndent;
  CNode *VtopNode;
  Zenum VnodeType;
  Zbool Vstatements;
  CListHead *VmemberList;
  CListHead *VimportList;
  char *VskipPredefined;
  char *VscopeName;
  CSymbol *Vclass;
  Zbool VinsideNew;
  char *VthisName;
  Zbool VinsideShared;
  char *VmoduleName;
  CSymbol *VreturnSymbol;
  CSymbol *VswitchSymbol;
  CListHead *VcaseList;
  Zbool Vinit;
  Zbool VforwardDeclare;
  Zbool VnoGenerate;
  CDictHead *VusedIdKeywords;
  CDictHead *VusedItems;
};
