#define INC_input_D 1
/*
 * DECLARE FUNCTIONS AND GLOBALS
 */
#ifndef INC_parse_D
#include "../ZUDIR/parse.d.h"
#endif
#ifndef INC_pos_D
#include "../ZUDIR/pos.d.h"
#endif
#ifndef INC_token_D
#include "../ZUDIR/token.d.h"
#endif
#ifndef INC_tokenize_D
#include "../ZUDIR/tokenize.d.h"
#endif
CInput *CInput__FNEW(FILE *A_fd, char *Afname);
Zint CInput__Fget(CInput *THIS);
void CInput__Fpush(CInput *THIS, Zint Ac);
CToken *CInput__FgetToken(CInput *THIS);
void CInput__FpushToken(CInput *THIS, CToken *Atoken);
void Iinput();
