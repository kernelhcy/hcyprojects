#define INC_input_S 1
/*
 * STRUCTS
 */
#ifndef INC_parse_S
#include "../ZUDIR/parse.s.h"
#endif
#ifndef INC_pos_S
#include "../ZUDIR/pos.s.h"
#endif
#ifndef INC_token_S
#include "../ZUDIR/token.s.h"
#endif
#ifndef INC_tokenize_S
#include "../ZUDIR/tokenize.s.h"
#endif
struct CInput__S {
  FILE *Vfd;
  CPos *Vpos;
  Zint VprevLineCol;
  CListHead *VcharStack;
  CListHead *VtokenStack;
  CDictHead *VusedIdKeywords;
};
