#define INC_zimbufile_B 1
/*
 * FUNCTION BODIES
 */
#ifndef INC_error_B
#include "../ZUDIR/error.b.c"
#endif
#ifndef INC_node_B
#include "../ZUDIR/node.b.c"
#endif
#ifndef INC_output_B
#include "../ZUDIR/output.b.c"
#endif
#ifndef INC_parse_B
#include "../ZUDIR/parse.b.c"
#endif
#ifndef INC_scope_B
#include "../ZUDIR/scope.b.c"
#endif
#ifndef INC_symbol_B
#include "../ZUDIR/symbol.b.c"
#endif
#ifndef INC_usedfile_B
#include "../ZUDIR/usedfile.b.c"
#endif
CZimbuFile *CZimbuFile__FNEW(char *A_filename) {
  CZimbuFile *THIS = Zalloc(sizeof(CZimbuFile));
  THIS->Vfilename = A_filename;
  if ((Zstrcmp(ZStringByteSlice(A_filename, -(3), -(1)), ".zu") == 0))
  {
    THIS->VrootName = ZStringByteSlice(A_filename, 0, -(4));
  }
else
  {
    THIS->VrootName = ZStringByteSlice(A_filename, 0, -(5));
  }
  Zint Vslash;
  Vslash = ZStringRindex(THIS->VrootName, 47);
  if ((Vslash >= 0))
  {
    THIS->VdirName = ZStringByteSlice(THIS->VrootName, 0, (Vslash - 1));
    THIS->VrootName = ZStringByteSlice(THIS->VrootName, (Vslash + 1), -(1));
  }
else
  {
    THIS->VdirName = "";
  }
  THIS->VinitFunc = Zconcat("I", THIS->VrootName);
  THIS->VstartedPass = -(1);
  THIS->Vc = CZimbuFile__X__CCodeSpecific__FNEW__1();
  THIS->Vjs = CZimbuFile__X__CCodeSpecific__FNEW__1();
  return THIS;
}
Zstatus CZimbuFile__Fparse(CZimbuFile *THIS, char *Aindent, CUsedFile *AusedFile) {
  if ((THIS->VstartedPass == -(1)))
  {
    THIS->VstartedPass = 0;
    THIS->VtopScope = MParse__FparseFile(THIS->Vfilename, Aindent, AusedFile);
  }
  return ((THIS->VtopScope == NULL)) ? (0) : (1);
}
char *CZimbuFile__FgetModuleName(CZimbuFile *THIS) {
  CNode *Vnode;
  Vnode = CZimbuFile__FgetModuleNode(THIS);
  if ((Vnode == NULL))
  {
    return NULL;
  }
  return Vnode->Vn_string;
}
CNode *CZimbuFile__FgetModuleNode(CZimbuFile *THIS) {
  CNode *Vnode;
  Vnode = THIS->VtopScope->VtopNode;
  while ((Vnode != NULL))
  {
    if ((Vnode->Vn_type == 19))
    {
      break;
    }
    Vnode = Vnode->Vn_next;
  }
  if ((Vnode == NULL))
  {
    MError__Freport("No Module found in JS import");
  }
  return Vnode;
}
CZimbuFile__X__CCodeSpecific *CZimbuFile__X__CCodeSpecific__FNEW__1() {
  CZimbuFile__X__CCodeSpecific *THIS = Zalloc(sizeof(CZimbuFile__X__CCodeSpecific));
  CZimbuFile__X__CCodeSpecific__Fclear(THIS);
  return THIS;
}
void CZimbuFile__X__CCodeSpecific__Fclear(CZimbuFile__X__CCodeSpecific *THIS) {
  THIS->Vheads = COutput__X__CHeads__FNEW__1();
  THIS->Voutputs = COutput__X__CGroup__FNEW__1();
  COutput__X__CGroup__FsetHeads(THIS->Voutputs, THIS->Vheads);
}
CZimbuFile *CZimbuFile__X__Ffind(CListHead *AimportList, char *Aname) {
  if ((AimportList != NULL))
  {
    {
      Zfor_T *Zf = ZforNew(AimportList, 2);
      CZimbuFile *Vimport;
      for (ZforGetPtr(Zf, (char **)&Vimport); ZforCont(Zf); ZforNextPtr(Zf, (char **)&Vimport)) {
        if ((Zstrcmp(Vimport->Vfilename, Aname) == 0))
        {
          return Vimport;
        }
      }
    }
  }
  return NULL;
}
void Izimbufile() {
 static int done = 0;
 if (!done) {
  done = 1;
  Ioutput();
  Iparse();
  Iscope();
  Isymbol();
  Iusedfile();
 }
}
