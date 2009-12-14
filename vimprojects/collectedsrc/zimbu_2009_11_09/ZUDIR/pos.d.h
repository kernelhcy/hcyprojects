#define INC_pos_D 1
/*
 * DECLARE FUNCTIONS AND GLOBALS
 */
CPos *CPos__FNEW(char *A_fname);
CPos *CPos__Fcopy(CPos *THIS);
void CPos__FnextLine(CPos *THIS);
char *CPos__FtoString(CPos *THIS);
