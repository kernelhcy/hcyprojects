#define INC_output_S 1
/*
 * STRUCTS
 */
#ifndef INC_scope_S
#include "../ZUDIR/scope.s.h"
#endif
struct COutput__X__CFragmentHead__S {
  CListHead *Vitems;
};
struct COutput__X__CHeads__S {
  COutput__X__CFragmentHead *Vtypedefs;
  COutput__X__CFragmentHead *Vstructs;
  COutput__X__CFragmentHead *Vdeclares;
  COutput__X__CFragmentHead *VfuncBodies;
  COutput__X__CFragmentHead *Vinits;
  COutput__X__CFragmentHead *VmainLines;
};
struct COutput__X__CGroup__S {
  COutput *Vout;
  COutput *VvarOut;
  COutput *VtypeOut;
  COutput *VstructOut;
  COutput *VdeclOut;
  COutput *VbodyOut;
  COutput *VinitOut;
  COutput *VmainOut;
  COutput *VorigBodyOut;
};
struct COutput__S {
  Zbool Vwriting;
  COutput__X__CFragmentHead *Vhead1;
  COutput__X__CFragmentHead *Vhead2;
};
