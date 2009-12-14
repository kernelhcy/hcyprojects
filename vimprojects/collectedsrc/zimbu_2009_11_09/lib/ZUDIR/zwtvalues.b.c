#define INC_zwtvalues_B 1
/*
 * FUNCTION BODIES
 */
void Izwtvalues() {
 static int done = 0;
 if (!done) {
  done = 1;
  MZWTvalues__VpermuNames = ZListAdd(ZListAdd(ZListAdd(ZListAdd(ZListAdd(ZListAdd(Zalloc(sizeof(CListHead)), -1, 0, "ie6", 1), -1, 0, "ie8", 1), -1, 0, "gecko", 1), -1, 0, "gecko18", 1), -1, 0, "safari", 1), -1, 0, "opera", 1);
 }
}
