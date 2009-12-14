#define INC_tokenize_D 1
/*
 * DECLARE FUNCTIONS AND GLOBALS
 */
#ifndef INC_error_D
#include "../ZUDIR/error.d.h"
#endif
#ifndef INC_input_D
#include "../ZUDIR/input.d.h"
#endif
#ifndef INC_token_D
#include "../ZUDIR/token.d.h"
#endif
#ifndef INC_pos_D
#include "../ZUDIR/pos.d.h"
#endif
#ifndef INC_parse_D
#include "../ZUDIR/parse.d.h"
#endif
Zbool MTokenize__FisIdChar(Zint Ac);
CDictHead *MTokenize__VthreeChar;
CDictHead *MTokenize__VtwoChar;
CDictHead *MTokenize__VoneChar;
CDictHead *MTokenize__Vkeywords;
CToken *MTokenize__Fget(CInput *Ain);
void MTokenize__FgetRawString(CInput *Ain, char * *Rbuffer, Zint AinitLen);
void MTokenize__FskipWhite(CInput *Ain, CToken *Ares);
void Itokenize();
