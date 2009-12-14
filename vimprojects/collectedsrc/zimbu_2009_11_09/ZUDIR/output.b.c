#define INC_output_B 1
/*
 * FUNCTION BODIES
 */
#ifndef INC_scope_B
#include "../ZUDIR/scope.b.c"
#endif
COutput *COutput__FNEW(COutput__X__CFragmentHead *Ahead) {
  COutput *THIS = Zalloc(sizeof(COutput));
  THIS->Vhead1 = Ahead;
  return THIS;
}
void COutput__Fwrite(COutput *THIS, char *As) {
  char *Vss;
  Vss = (((As == NULL))) ? ("NIL") : (As);
  if (THIS->Vwriting)
  {
    if ((THIS->Vhead1 != NULL))
    {
      COutput__X__CFragmentHead__Fadd(THIS->Vhead1, Vss);
    }
    if ((THIS->Vhead2 != NULL))
    {
      COutput__X__CFragmentHead__Fadd(THIS->Vhead2, Vss);
    }
  }
}
void COutput__Fprepend(COutput *THIS, char *As) {
  char *Vss;
  Vss = (((As == NULL))) ? ("NIL") : (As);
  if (THIS->Vwriting)
  {
    if ((THIS->Vhead1 != NULL))
    {
      COutput__X__CFragmentHead__Finsert(THIS->Vhead1, Vss);
    }
    if ((THIS->Vhead2 != NULL))
    {
      COutput__X__CFragmentHead__Finsert(THIS->Vhead2, Vss);
    }
  }
}
void COutput__Fappend(COutput *THIS, COutput *Aout) {
  if ((THIS->Vwriting && (Aout->Vhead1 != NULL)))
  {
    if ((THIS->Vhead1 != NULL))
    {
      COutput__X__CFragmentHead__Fappend__1(THIS->Vhead1, Aout->Vhead1);
    }
    if ((THIS->Vhead2 != NULL))
    {
      COutput__X__CFragmentHead__Fappend__1(THIS->Vhead2, Aout->Vhead1);
    }
  }
}
void COutput__FwriteIndent(COutput *THIS, Zint Adepth) {
  if (THIS->Vwriting)
  {
    Zint Vi;
    Vi = 0;
    while ((Vi < Adepth))
    {
      COutput__Fwrite(THIS, "  ");
      ++(Vi);
    }
  }
}
char *COutput__FtoString(COutput *THIS) {
  if ((THIS->Vhead1 == NULL))
  {
    return "";
  }
  return COutput__X__CFragmentHead__FtoString__1(THIS->Vhead1);
}
COutput *COutput__Fcopy(COutput *THIS) {
  COutput *VblockOut;
  VblockOut = COutput__FNEW(NULL);
  VblockOut->Vwriting = THIS->Vwriting;
  if ((THIS->Vhead1 != NULL))
  {
    VblockOut->Vhead1 = COutput__X__CFragmentHead__Fcopy__1(THIS->Vhead1);
  }
  if ((THIS->Vhead2 != NULL))
  {
    VblockOut->Vhead2 = COutput__X__CFragmentHead__Fcopy__1(THIS->Vhead2);
  }
  return VblockOut;
}
void COutput__Fclear(COutput *THIS) {
  if ((THIS->Vhead1 != NULL))
  {
    COutput__X__CFragmentHead__Fclear__1(THIS->Vhead1);
  }
  if ((THIS->Vhead2 != NULL))
  {
    COutput__X__CFragmentHead__Fclear__1(THIS->Vhead2);
  }
}
void COutput__Freset(COutput *THIS, COutput *Asrc) {
  THIS->Vwriting = Asrc->Vwriting;
  if ((Asrc->Vhead1 == NULL))
  {
    THIS->Vhead1 = NULL;
  }
else
  {
    THIS->Vhead1 = COutput__X__CFragmentHead__Fcopy__1(Asrc->Vhead1);
  }
  if ((Asrc->Vhead2 == NULL))
  {
    THIS->Vhead2 = NULL;
  }
else
  {
    THIS->Vhead2 = COutput__X__CFragmentHead__Fcopy__1(Asrc->Vhead2);
  }
}
COutput__X__CFragmentHead *COutput__X__CFragmentHead__FNEW__1() {
  COutput__X__CFragmentHead *THIS = Zalloc(sizeof(COutput__X__CFragmentHead));
  THIS->Vitems = Zalloc(sizeof(CListHead));
  return THIS;
}
void COutput__X__CFragmentHead__Fadd(COutput__X__CFragmentHead *THIS, char *Atext) {
  ZListAdd(THIS->Vitems, -1, 0, Atext, 1);
}
void COutput__X__CFragmentHead__Fappend__1(COutput__X__CFragmentHead *THIS, COutput__X__CFragmentHead *Ahead) {
  if ((Ahead->Vitems != NULL))
  {
    ZListExtend(THIS->Vitems, Ahead->Vitems);
  }
}
void COutput__X__CFragmentHead__Finsert(COutput__X__CFragmentHead *THIS, char *Atext) {
  ZListInsert(THIS->Vitems, 0, 0, Atext, 1);
}
COutput__X__CFragmentHead *COutput__X__CFragmentHead__Fcopy__1(COutput__X__CFragmentHead *THIS) {
  COutput__X__CFragmentHead *Vlh;
  Vlh = COutput__X__CFragmentHead__FNEW__1();
  Vlh->Vitems = THIS->Vitems;
  return Vlh;
}
void COutput__X__CFragmentHead__Fwrite__1(COutput__X__CFragmentHead *THIS, FILE *Afd) {
  {
    Zfor_T *Zf = ZforNew(THIS->Vitems, 2);
    char *Vs;
    for (ZforGetPtr(Zf, (char **)&Vs); ZforCont(Zf); ZforNextPtr(Zf, (char **)&Vs)) {
      fputs(Vs, Afd);
    }
  }
}
void COutput__X__CFragmentHead__Fclear__1(COutput__X__CFragmentHead *THIS) {
  THIS->Vitems = Zalloc(sizeof(CListHead));
}
char *COutput__X__CFragmentHead__FtoString__1(COutput__X__CFragmentHead *THIS) {
  char *Vret;
  Vret = "";
  {
    Zfor_T *Zf = ZforNew(THIS->Vitems, 2);
    char *Vs;
    for (ZforGetPtr(Zf, (char **)&Vs); ZforCont(Zf); ZforNextPtr(Zf, (char **)&Vs)) {
      Vret = Zconcat(Vret, Vs);
    }
  }
  return Vret;
}
Zbool COutput__X__CFragmentHead__Fempty(COutput__X__CFragmentHead *THIS) {
  {
    Zfor_T *Zf = ZforNew(THIS->Vitems, 2);
    char *Vs;
    for (ZforGetPtr(Zf, (char **)&Vs); ZforCont(Zf); ZforNextPtr(Zf, (char **)&Vs)) {
      if (((Vs != NULL) && (Zstrcmp(Vs, "") != 0)))
      {
        return 0;
      }
    }
  }
  return 1;
}
COutput__X__CHeads *COutput__X__CHeads__FNEW__1() {
  COutput__X__CHeads *THIS = Zalloc(sizeof(COutput__X__CHeads));
  THIS->Vtypedefs = COutput__X__CFragmentHead__FNEW__1();
  THIS->Vstructs = COutput__X__CFragmentHead__FNEW__1();
  THIS->Vdeclares = COutput__X__CFragmentHead__FNEW__1();
  THIS->VfuncBodies = COutput__X__CFragmentHead__FNEW__1();
  THIS->Vinits = COutput__X__CFragmentHead__FNEW__1();
  THIS->VmainLines = COutput__X__CFragmentHead__FNEW__1();
  return THIS;
}
COutput__X__CGroup *COutput__X__CGroup__FNEW__1() {
  COutput__X__CGroup *THIS = Zalloc(sizeof(COutput__X__CGroup));
  return THIS;
}
COutput__X__CGroup *COutput__X__CGroup__FNEW__2(COutput__X__CHeads *Aheads) {
  COutput__X__CGroup *THIS = Zalloc(sizeof(COutput__X__CGroup));
  COutput__X__CGroup__FsetHeads(THIS, Aheads);
  return THIS;
}
void COutput__X__CGroup__FsetHeads(COutput__X__CGroup *THIS, COutput__X__CHeads *Aheads) {
  THIS->VtypeOut = COutput__FNEW(Aheads->Vtypedefs);
  THIS->VstructOut = COutput__FNEW(Aheads->Vstructs);
  THIS->VdeclOut = COutput__FNEW(Aheads->Vdeclares);
  THIS->VbodyOut = COutput__FNEW(Aheads->VfuncBodies);
  THIS->VinitOut = COutput__FNEW(Aheads->Vinits);
  THIS->VmainOut = COutput__FNEW(Aheads->VmainLines);
  THIS->Vout = THIS->VmainOut;
  THIS->VvarOut = THIS->VdeclOut;
}
void COutput__X__CGroup__FstartWriting(COutput__X__CGroup *THIS) {
  THIS->VtypeOut->Vwriting = 1;
  THIS->VstructOut->Vwriting = 1;
  THIS->VdeclOut->Vwriting = 1;
  THIS->VbodyOut->Vwriting = 1;
  THIS->VinitOut->Vwriting = 1;
  THIS->VmainOut->Vwriting = 1;
}
COutput__X__CGroup *COutput__X__CGroup__Fcopy__1(COutput__X__CGroup *THIS) {
  COutput__X__CGroup *Vouts;
  Vouts = COutput__X__CGroup__FNEW__1();
  Vouts->Vout = THIS->Vout;
  Vouts->VvarOut = THIS->VvarOut;
  Vouts->VtypeOut = THIS->VtypeOut;
  Vouts->VstructOut = THIS->VstructOut;
  Vouts->VdeclOut = THIS->VdeclOut;
  Vouts->VbodyOut = THIS->VbodyOut;
  Vouts->VinitOut = THIS->VinitOut;
  Vouts->VmainOut = THIS->VmainOut;
  return Vouts;
}
void Ioutput() {
 static int done = 0;
 if (!done) {
  done = 1;
  Iscope();
  COutput__X__VnoOut = COutput__FNEW(NULL);
 }
}
