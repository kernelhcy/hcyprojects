#define INC_token_D 1
/*
 * DECLARE FUNCTIONS AND GLOBALS
 */
#ifndef INC_error_D
#include "../ZUDIR/error.d.h"
#endif
#ifndef INC_pos_D
#include "../ZUDIR/pos.d.h"
#endif
void CToken__Ferror(CToken *THIS, char *Amsg);
void CToken__FerrorNotAllowed(CToken *THIS);
Zbool CToken__FisSep(CToken *THIS);
CToken *CToken__Fcopy(CToken *THIS);
