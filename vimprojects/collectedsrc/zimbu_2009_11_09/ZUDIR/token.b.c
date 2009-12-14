#define INC_token_B 1
/*
 * FUNCTION BODIES
 */
#ifndef INC_error_B
#include "../ZUDIR/error.b.c"
#endif
#ifndef INC_pos_B
#include "../ZUDIR/pos.b.c"
#endif
void CToken__Ferror(CToken *THIS, char *Amsg) {
  MError__Freport__1(Amsg, THIS->VstartPos);
}
void CToken__FerrorNotAllowed(CToken *THIS) {
  MError__Freport__1(Zconcat(Zconcat("'", THIS->Vvalue), "' not allowed here"), THIS->VstartPos);
}
Zbool CToken__FisSep(CToken *THIS) {
  return (((THIS->Vtype == 2) || (THIS->Vtype == 3)) || (THIS->Vtype == 4));
}
CToken *CToken__Fcopy(CToken *THIS) {
  CToken *Vt;
  Vt = Zalloc(sizeof(CToken));
  Vt->Vtype = THIS->Vtype;
  Vt->Vvalue = THIS->Vvalue;
  if ((THIS->VstartPos != NULL))
  {
    Vt->VstartPos = CPos__Fcopy(THIS->VstartPos);
  }
  if ((THIS->VendPos != NULL))
  {
    Vt->VendPos = CPos__Fcopy(THIS->VendPos);
  }
  return Vt;
}
