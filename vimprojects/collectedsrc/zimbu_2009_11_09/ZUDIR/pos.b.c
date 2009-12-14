#define INC_pos_B 1
/*
 * FUNCTION BODIES
 */
CPos *CPos__FNEW(char *A_fname) {
  CPos *THIS = Zalloc(sizeof(CPos));
  THIS->Vfname = A_fname;
  THIS->Vlnum = 1;
  THIS->Vcol = 1;
  return THIS;
}
CPos *CPos__Fcopy(CPos *THIS) {
  CPos *Vp;
  Vp = CPos__FNEW(THIS->Vfname);
  Vp->Vlnum = THIS->Vlnum;
  Vp->Vcol = THIS->Vcol;
  return Vp;
}
void CPos__FnextLine(CPos *THIS) {
  ++(THIS->Vlnum);
  THIS->Vcol = 1;
}
char *CPos__FtoString(CPos *THIS) {
  return Zconcat(Zconcat(Zconcat(Zconcat(THIS->Vfname, " line "), Zint2string(THIS->Vlnum)), " col "), Zint2string(THIS->Vcol));
}
