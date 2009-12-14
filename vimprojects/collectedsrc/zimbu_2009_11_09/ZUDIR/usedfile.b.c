#define INC_usedfile_B 1
/*
 * FUNCTION BODIES
 */
#ifndef INC_builtin_B
#include "../ZUDIR/builtin.b.c"
#endif
#ifndef INC_scope_B
#include "../ZUDIR/scope.b.c"
#endif
#ifndef INC_zimbufile_B
#include "../ZUDIR/zimbufile.b.c"
#endif
CUsedFile *CUsedFile__FNEW(char *AfileName, Zbits Aflags) {
  CUsedFile *THIS = Zalloc(sizeof(CUsedFile));
  THIS->VzimbuFile = CZimbuFile__FNEW(AfileName);
  THIS->VisMainFile = (((Aflags) & 1));
  THIS->VisTopFile = (((Aflags) & 2));
  if (THIS->VisTopFile)
  {
    THIS->VusedImports = ZnewDict(1);
  }
  if ((((Aflags) & 4)))
  {
    THIS->VzimbuFile->VusedAsZwt = 1;
  }
else
  {
    THIS->VzimbuFile->VusedAsZimbu = 1;
  }
  return THIS;
}
CUsedFile *CUsedFile__FNEW__1(CZimbuFile *A_zimbuFile, Zbool A_isMainFile, Zbool A_isTopFile) {
  CUsedFile *THIS = Zalloc(sizeof(CUsedFile));
  THIS->VzimbuFile = A_zimbuFile;
  THIS->VisMainFile = A_isMainFile;
  THIS->VisTopFile = A_isTopFile;
  if (THIS->VisTopFile)
  {
    THIS->VusedImports = ZnewDict(1);
  }
  return THIS;
}
Zstatus CUsedFile__Fparse(CUsedFile *THIS, char *Aindent) {
  Zstatus Vret;
  Vret = CZimbuFile__Fparse(THIS->VzimbuFile, Aindent, THIS);
  if ((THIS->VzimbuFile->VtopScope != NULL))
  {
    if (THIS->VisTopFile)
    {
      THIS->VusedBuiltins = ZnewDict(1);
    }
    CBuiltin__X__FcheckBuiltin(THIS);
  }
  return Vret;
}
CScope *CUsedFile__Fscope(CUsedFile *THIS) {
  return THIS->VzimbuFile->VtopScope;
}
CUsedFile *CUsedFile__FfindTopFile(CUsedFile *THIS) {
  if (THIS->VisTopFile)
  {
    return THIS;
  }
  if ((THIS->Vparent != NULL))
  {
    return CUsedFile__FfindTopFile(THIS->Vparent);
  }
  return NULL;
}
Zbool CUsedFile__FisNewImport(CUsedFile *THIS, CZimbuFile *Azf) {
  CUsedFile *Vtop;
  Vtop = CUsedFile__FfindTopFile(THIS);
  if ((Vtop == NULL))
  {
    (fputs("INTERNAL: can't find top UsedFile", stdout) | fputc('\n', stdout));
    return 1;
  }
  CZimbuFile *Vexisting;
  Vexisting = ZDictGetPtrDef(Vtop->VusedImports, 0, Azf->Vfilename, NULL);
  if ((Vexisting == NULL))
  {
    ZDictAdd(1, Vtop->VusedImports, 0, Azf->Vfilename, 0, Azf);
    return 1;
  }
  return 0;
}
void Iusedfile() {
 static int done = 0;
 if (!done) {
  done = 1;
  Ibuiltin();
  Iscope();
  Izimbufile();
 }
}
