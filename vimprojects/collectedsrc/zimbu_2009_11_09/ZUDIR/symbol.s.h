#define INC_symbol_S 1
/*
 * STRUCTS
 */
#ifndef INC_attr_S
#include "../ZUDIR/attr.s.h"
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
#ifndef INC_pos_S
#include "../ZUDIR/pos.s.h"
#endif
#ifndef INC_resolve_S
#include "../ZUDIR/resolve.s.h"
#endif
#ifndef INC_scontext_S
#include "../ZUDIR/scontext.s.h"
#endif
#ifndef INC_scope_S
#include "../ZUDIR/scope.s.h"
#endif
struct CSymbol__S {
  char *Vname;
  char *VcName;
  Zenum Vtype;
  Zint Vvalue;
  Zint Vmask;
  Zbool Vused;
  CListHead *VmemberList;
  CScope *Vscope;
  CListHead *Vchildren;
  CSymbol *Vclass;
  char *VclassName;
  Zbool VnoGenerate;
  CSymbol *VparentClass;
  CListHead *Vinterfaces;
  COutput *VstructOut;
  CSymbol *VkeySymbol;
  CSymbol *VreturnSymbol;
  Zbbits Vattributes;
  CNode *Vnode;
  CPos *Vpos;
  Zenum Vtlang;
  CDictHead *VzwtPermu;
  void *Vproduce;
};
