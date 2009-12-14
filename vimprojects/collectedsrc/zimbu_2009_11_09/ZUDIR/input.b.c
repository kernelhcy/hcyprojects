#define INC_input_B 1
/*
 * FUNCTION BODIES
 */
#ifndef INC_parse_B
#include "../ZUDIR/parse.b.c"
#endif
#ifndef INC_pos_B
#include "../ZUDIR/pos.b.c"
#endif
#ifndef INC_token_B
#include "../ZUDIR/token.b.c"
#endif
#ifndef INC_tokenize_B
#include "../ZUDIR/tokenize.b.c"
#endif
CInput *CInput__FNEW(FILE *A_fd, char *Afname) {
  CInput *THIS = Zalloc(sizeof(CInput));
  THIS->Vfd = A_fd;
  THIS->Vpos = CPos__FNEW(Afname);
  THIS->VcharStack = Zalloc(sizeof(CListHead));
  THIS->VtokenStack = Zalloc(sizeof(CListHead));
  THIS->VusedIdKeywords = ZnewDict(1);
  return THIS;
}
Zint CInput__Fget(CInput *THIS) {
  Zint Vc;
  Vc = 0;
  if ((THIS->VcharStack->itemCount > 0))
  {
    Vc = ZListPopIntItem(THIS->VcharStack, -1);
    ++(THIS->Vpos->Vcol);
  }
else
  {
    do
    {
      Vc = fgetc(THIS->Vfd);
      ++(THIS->Vpos->Vcol);
      if (((((Vc >= 0) && (Vc <= 31))) || (Vc == 127)))
      {
        if ((Vc == 0))
        {
          MParse__Ferror("found NUL character", THIS);
          Vc = 32;
        }
      else if ((Vc == 9))
        {
          MParse__Ferror("found Tab character", THIS);
          Vc = 32;
        }
      else if (((Vc != 13) && (Vc != 10)))
        {
          MParse__Ferror("found control character", THIS);
          Vc = 32;
        }
      }
    }
      while (!(((Vc != 13) && (Vc != 65279))));
  }
  if ((Vc == 10))
  {
    THIS->VprevLineCol = THIS->Vpos->Vcol;
    CPos__FnextLine(THIS->Vpos);
  }
  return Vc;
}
void CInput__Fpush(CInput *THIS, Zint Ac) {
  ZListAdd(THIS->VcharStack, -1, Ac, NULL, 0);
  if ((Ac == 10))
  {
    THIS->Vpos->Vcol = THIS->VprevLineCol;
    --(THIS->Vpos->Vlnum);
  }
else
  {
    --(THIS->Vpos->Vcol);
  }
}
CToken *CInput__FgetToken(CInput *THIS) {
  if ((THIS->VtokenStack->itemCount > 0))
  {
    return ZListPopPtrItem(THIS->VtokenStack, -1);
  }
  return MTokenize__Fget(THIS);
}
void CInput__FpushToken(CInput *THIS, CToken *Atoken) {
  ZListAdd(THIS->VtokenStack, -1, 0, Atoken, 1);
}
void Iinput() {
 static int done = 0;
 if (!done) {
  done = 1;
  Itokenize();
 }
}
