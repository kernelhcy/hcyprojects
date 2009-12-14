#define INC_zwtSimple_B 1
/*
 * FUNCTION BODIES
 */
CZwtSimple *CZwtSimple__X__FgetOne(char *Aval) {
  CZwtSimple *Vz;
  Vz = Zalloc(sizeof(CZwtSimple));
  Vz->Vdummy = Aval;
  return Vz;
}
