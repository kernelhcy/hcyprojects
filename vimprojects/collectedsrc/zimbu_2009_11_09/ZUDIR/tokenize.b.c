#define INC_tokenize_B 1
/*
 * FUNCTION BODIES
 */
#ifndef INC_error_B
#include "../ZUDIR/error.b.c"
#endif
#ifndef INC_input_B
#include "../ZUDIR/input.b.c"
#endif
#ifndef INC_token_B
#include "../ZUDIR/token.b.c"
#endif
#ifndef INC_pos_B
#include "../ZUDIR/pos.b.c"
#endif
#ifndef INC_parse_B
#include "../ZUDIR/parse.b.c"
#endif
Zbool MTokenize__FisIdChar(Zint Ac) {
  return ((((((Ac >= 97) && (Ac <= 122))) || (((Ac >= 65) && (Ac <= 90)))) || (((Ac >= 48) && (Ac <= 57)))) || (Ac == 95));
}
CToken *MTokenize__Fget(CInput *Ain) {
  Zint Vi;
  Vi = 0;
  Zint Vc;
  Vc = 0;
  char *Vp;
  Vp = NULL;
  CToken *Vres;
  Vres = Zalloc(sizeof(CToken));
  Vres->VstartPos = CPos__Fcopy(Ain->Vpos);
  MTokenize__FskipWhite(Ain, Vres);
  if (1)
  {
    if ((Vres->Vtype != 5))
    {
      Vres->VendPos = CPos__Fcopy(Ain->Vpos);
      return Vres;
    }
  }
  Vres->VstartPos = CPos__Fcopy(Ain->Vpos);
  Zint Vlen;
  Vlen = 300;
  char *Vbuffer;
  Vbuffer = malloc(Vlen);
  Vbuffer[0] = CInput__Fget(Ain);
  Vbuffer[1] = CInput__Fget(Ain);
  Vbuffer[2] = CInput__Fget(Ain);
  Vbuffer[3] = 0;
  Vres->Vtype = ZDictGetIntDef(MTokenize__VthreeChar, 0, Vbuffer, 0);
  if ((Vres->Vtype == 0))
  {
    CInput__Fpush(Ain, Vbuffer[2]);
    Vbuffer[2] = 0;
    Vres->Vtype = ZDictGetIntDef(MTokenize__VtwoChar, 0, Vbuffer, 0);
    if ((Vres->Vtype == 14))
    {
      MTokenize__FgetRawString(Ain, &(Vbuffer), Vlen);
      Vres->Vtype = 13;
    }
  else if ((Vres->Vtype == 0))
    {
      CInput__Fpush(Ain, Vbuffer[1]);
      Vbuffer[1] = 0;
      Vres->Vtype = ZDictGetIntDef(MTokenize__VoneChar, Vbuffer[0], NULL, 0);
      if ((Vres->Vtype == 0))
      {
        switch (Vbuffer[0])
        {
        case 34:
          {
            {
              Vres->Vtype = 13;
              while (1)
              {
                Vc = CInput__Fget(Ain);
                if ((Vc == 34))
                {
                  break;
                }
                if (((Vc == 10) || (Vc == MIO__Veof)))
                {
                  CInput__Fpush(Ain, Vc);
                  MParse__Ferror("missing double quote", Ain);
                  break;
                }
                if (((Vi + 3) >= Vlen))
                {
                  Vlen += 300;
                  Vbuffer = realloc(Vbuffer, Vlen);
                }
                if ((Vc == 92))
                {
                  Vc = CInput__Fget(Ain);
                  if (((Vc == 10) || (Vc == MIO__Veof)))
                  {
                    MParse__Ferror("missing double quote", Ain);
                    break;
                  }
                  if ((Vc == 110))
                  {
                    Vbuffer[(Vi)++] = 10;
                  }
                else if ((Vc == 116))
                  {
                    Vbuffer[(Vi)++] = 9;
                  }
                else if ((Vc == 114))
                  {
                    Vbuffer[(Vi)++] = 13;
                  }
                else
                  {
                    Vbuffer[(Vi)++] = Vc;
                  }
                }
              else
                {
                  Vbuffer[(Vi)++] = Vc;
                }
              }
              Vbuffer[Vi] = 0;
            }
              break;
          }
        case 39:
          {
            {
              Vc = CInput__Fget(Ain);
              if ((Vc == 39))
              {
                Vc = CInput__Fget(Ain);
                if ((Vc == 39))
                {
                  Vres->Vtype = 13;
                  while (1)
                  {
                    Vc = CInput__Fget(Ain);
                    if ((Vc == 39))
                    {
                      Vc = CInput__Fget(Ain);
                      if ((Vc == 39))
                      {
                        Vc = CInput__Fget(Ain);
                        if ((Vc == 39))
                        {
                          break;
                        }
                        CInput__Fpush(Ain, Vc);
                        Vc = 39;
                      }
                      CInput__Fpush(Ain, Vc);
                      Vc = 39;
                    }
                    if ((Vc == MIO__Veof))
                    {
                      MParse__Ferror("missing end of ''' string", Ain);
                      break;
                    }
                    if (((Vi + 3) >= Vlen))
                    {
                      Vlen += 300;
                      Vbuffer = realloc(Vbuffer, Vlen);
                    }
                    Vbuffer[(Vi)++] = Vc;
                  }
                  Vbuffer[Vi] = 0;
                  break;
                }
              else
                {
                  CInput__Fpush(Ain, Vc);
                  Vc = 39;
                }
              }
              Vres->Vtype = 15;
              if ((Vc == 92))
              {
                Vc = CInput__Fget(Ain);
                switch (Vc)
                {
                case 110:
                  {
                    {
                      Vc = 10;
                    }
                      break;
                  }
                case 114:
                  {
                    {
                      Vc = 13;
                    }
                      break;
                  }
                case 116:
                  {
                    {
                      Vc = 9;
                    }
                      break;
                  }
                }
              }
              Vbuffer[0] = Vc;
              Vbuffer[1] = 0;
              Vc = CInput__Fget(Ain);
              if ((Vc != 39))
              {
                MParse__Ferror("missing single quote", Ain);
              }
            }
              break;
          }
        default:
          {
            {
              Vc = Vbuffer[0];
              if (!(MTokenize__FisIdChar(Vc)))
              {
                MParse__Ferror(Zconcat(Zconcat("Unrecognized character: '", Ztochar(Vc)), "'"), Ain);
                Vi = 1;
              }
            else
              {
                Zint Vquot;
                Vquot = 0;
                if (((Vc >= 48) && (Vc <= 57)))
                {
                  Vquot = 39;
                }
                while (((Vi < (Vlen - 2)) && ((MTokenize__FisIdChar(Vc) || (Vc == Vquot)))))
                {
                  Vbuffer[(Vi)++] = Vc;
                  Vc = CInput__Fget(Ain);
                }
                CInput__Fpush(Ain, Vc);
              }
              Vbuffer[Vi] = 0;
              Vres->Vtype = ZDictGetIntDef(MTokenize__Vkeywords, 0, Vbuffer, 0);
              if ((Vres->Vtype != 0))
              {
                if (((Vres->Vtype == 18) && !(ZDictHas(Ain->VusedIdKeywords, 0, Vbuffer))))
                {
                  ZDictAdd(1, Ain->VusedIdKeywords, 0, Vbuffer, 1, NULL);
                }
              }
            else
              {
                Zbool VallUpper;
                VallUpper = 1;
                Zbool VdoubleUnderscore;
                VdoubleUnderscore = 0;
                Vi = 0;
                Vc = Vbuffer[Vi];
                while ((Vc != 0))
                {
                  if (((((Vc < 65) || (Vc > 90))) && (Vc != 95)))
                  {
                    VallUpper = 0;
                  }
                  if (((Vc == 95) && (Vbuffer[(Vi + 1)] == 95)))
                  {
                    VdoubleUnderscore = 1;
                  }
                  Vc = Vbuffer[++(Vi)];
                }
                if (VallUpper)
                {
                  CToken__Ferror(Vres, Zconcat(Zconcat("Unrecognized keyword: '", Vbuffer), "'"));
                }
                if (VdoubleUnderscore)
                {
                  CToken__Ferror(Vres, Zconcat(Zconcat("'__' is illegal in identifier: '", Vbuffer), "'"));
                }
                Vres->Vtype = 18;
              }
            }
              break;
          }
        }
      }
    }
  }
  Vres->Vvalue = Vbuffer;
  Vres->VendPos = CPos__Fcopy(Ain->Vpos);
  return Vres;
}
void MTokenize__FgetRawString(CInput *Ain, char * *Rbuffer, Zint AinitLen) {
  Zint Vi;
  Vi = 0;
  Zint Vlen;
  Vlen = AinitLen;
  while (1)
  {
    Zint Vc;
    Vc = CInput__Fget(Ain);
    if ((Vc == 34))
    {
      Vc = CInput__Fget(Ain);
      if ((Vc != 34))
      {
        CInput__Fpush(Ain, Vc);
        break;
      }
    }
    if (((Vc == 10) || (Vc == MIO__Veof)))
    {
      CInput__Fpush(Ain, Vc);
      MParse__Ferror("missing double quote", Ain);
      break;
    }
    if (((Vi + 3) >= Vlen))
    {
      Vlen += 300;
      (*Rbuffer) = realloc((*Rbuffer), Vlen);
    }
    (*Rbuffer)[(Vi)++] = Vc;
  }
  (*Rbuffer)[Vi] = 0;
}
void MTokenize__FskipWhite(CInput *Ain, CToken *Ares) {
  Zint Vlen;
  Vlen = 0;
  Zint Vidx;
  Vidx = 0;
  char *Vbuffer;
  Vbuffer = NULL;
  Zbool VisSep;
  VisSep = 0;
  Zbool VhasLineBreak;
  VhasLineBreak = 0;
  Zbool VhasComment;
  VhasComment = 0;
  Zint Vc;
  Vc = CInput__Fget(Ain);
  if (((Vc == 32) || (Vc == 10)))
  {
    VisSep = 1;
  }
  while ((((Vc == 35) || (Vc == 32)) || (Vc == 10)))
  {
    if ((Vlen == 0))
    {
      Vlen = 100;
      Vbuffer = malloc(Vlen);
    }
  else if (((Vidx + 3) >= Vlen))
    {
      Vlen += 200;
      Vbuffer = realloc(Vbuffer, Vlen);
    }
    Vbuffer[(Vidx)++] = Vc;
    if ((Vc == 10))
    {
      VhasLineBreak = 1;
    }
  else if ((Vc == 35))
    {
      VhasComment = 1;
      do
      {
        Vc = CInput__Fget(Ain);
        if (((Vidx + 3) >= Vlen))
        {
          Vlen += 200;
          Vbuffer = realloc(Vbuffer, Vlen);
        }
        Vbuffer[(Vidx)++] = Vc;
      }
        while (!(((Vc == MIO__Veof) || (Vc == 10))));
    }
    Vc = CInput__Fget(Ain);
  }
  CInput__Fpush(Ain, Vc);
  if (VisSep)
  {
    if ((VhasLineBreak || VhasComment))
    {
      Ares->Vtype = 3;
    }
  else
    {
      Ares->Vtype = 2;
    }
  }
else if (VhasComment)
  {
    Ares->Vtype = 4;
  }
else
  {
    Ares->Vtype = 5;
  }
  if ((Vbuffer != NULL))
  {
    Vbuffer[Vidx] = 0;
    Ares->Vvalue = Vbuffer;
  }
}
void Itokenize() {
 static int done = 0;
 if (!done) {
  done = 1;
  MTokenize__VthreeChar = ZDictAdd(0, ZDictAdd(0, ZDictAdd(0, ZDictAdd(0, ZnewDict(1), 0, "..=", 96, NULL), 0, "<<=", 97, NULL), 0, ">>=", 98, NULL), 0, ">>>", 21, NULL);
  MTokenize__VtwoChar = ZDictAdd(0, ZDictAdd(0, ZDictAdd(0, ZDictAdd(0, ZDictAdd(0, ZDictAdd(0, ZDictAdd(0, ZDictAdd(0, ZDictAdd(0, ZDictAdd(0, ZDictAdd(0, ZDictAdd(0, ZDictAdd(0, ZDictAdd(0, ZDictAdd(0, ZDictAdd(0, ZDictAdd(0, ZDictAdd(0, ZDictAdd(0, ZDictAdd(0, ZDictAdd(0, ZnewDict(1), 0, "!=", 100, NULL), 0, "--", 73, NULL), 0, "-=", 87, NULL), 0, "++", 75, NULL), 0, "+=", 88, NULL), 0, "*=", 89, NULL), 0, "/=", 90, NULL), 0, "%=", 91, NULL), 0, "~=", 92, NULL), 0, "&=", 93, NULL), 0, "|=", 94, NULL), 0, "^=", 95, NULL), 0, "==", 99, NULL), 0, "<<", 85, NULL), 0, ">>", 84, NULL), 0, "<=", 107, NULL), 0, ">=", 105, NULL), 0, "&&", 101, NULL), 0, "||", 102, NULL), 0, "..", 80, NULL), 0, "R\"", 14, NULL);
  MTokenize__VoneChar = ZDictAdd(0, ZDictAdd(0, ZDictAdd(0, ZDictAdd(0, ZDictAdd(0, ZDictAdd(0, ZDictAdd(0, ZDictAdd(0, ZDictAdd(0, ZDictAdd(0, ZDictAdd(0, ZDictAdd(0, ZDictAdd(0, ZDictAdd(0, ZDictAdd(0, ZDictAdd(0, ZDictAdd(0, ZDictAdd(0, ZDictAdd(0, ZDictAdd(0, ZDictAdd(0, ZDictAdd(0, ZDictAdd(0, ZDictAdd(0, ZDictAdd(0, ZnewDict(0), MIO__Veof, NULL, 1, NULL), 123, NULL, 68, NULL), 125, NULL, 69, NULL), 40, NULL, 66, NULL), 41, NULL, 67, NULL), 91, NULL, 70, NULL), 93, NULL, 71, NULL), 44, NULL, 61, NULL), 46, NULL, 65, NULL), 126, NULL, 76, NULL), 47, NULL, 78, NULL), 42, NULL, 77, NULL), 37, NULL, 79, NULL), 59, NULL, 62, NULL), 63, NULL, 64, NULL), 58, NULL, 63, NULL), 94, NULL, 82, NULL), 45, NULL, 72, NULL), 43, NULL, 74, NULL), 61, NULL, 86, NULL), 60, NULL, 106, NULL), 62, NULL, 104, NULL), 33, NULL, 103, NULL), 38, NULL, 83, NULL), 124, NULL, 81, NULL);
  MTokenize__Vkeywords = ZDictAdd(0, ZDictAdd(0, ZDictAdd(0, ZDictAdd(0, ZDictAdd(0, ZDictAdd(0, ZDictAdd(0, ZDictAdd(0, ZDictAdd(0, ZDictAdd(0, ZDictAdd(0, ZDictAdd(0, ZDictAdd(0, ZDictAdd(0, ZDictAdd(0, ZDictAdd(0, ZDictAdd(0, ZDictAdd(0, ZDictAdd(0, ZDictAdd(0, ZDictAdd(0, ZDictAdd(0, ZDictAdd(0, ZDictAdd(0, ZDictAdd(0, ZDictAdd(0, ZDictAdd(0, ZDictAdd(0, ZDictAdd(0, ZDictAdd(0, ZDictAdd(0, ZDictAdd(0, ZDictAdd(0, ZDictAdd(0, ZDictAdd(0, ZDictAdd(0, ZDictAdd(0, ZDictAdd(0, ZDictAdd(0, ZDictAdd(0, ZDictAdd(0, ZDictAdd(0, ZDictAdd(0, ZDictAdd(0, ZDictAdd(0, ZDictAdd(0, ZDictAdd(0, ZDictAdd(0, ZDictAdd(0, ZDictAdd(0, ZDictAdd(0, ZDictAdd(0, ZDictAdd(0, ZDictAdd(0, ZDictAdd(0, ZDictAdd(0, ZDictAdd(0, ZDictAdd(0, ZDictAdd(0, ZDictAdd(0, ZDictAdd(0, ZDictAdd(0, ZnewDict(1), 0, "AND", 101, NULL), 0, "ANY", 16, NULL), 0, "ARG", 18, NULL), 0, "BITS", 30, NULL), 0, "BREAK", 51, NULL), 0, "CASE", 55, NULL), 0, "C", 22, NULL), 0, "CLASS", 25, NULL), 0, "CONTINUE", 52, NULL), 0, "COPY", 42, NULL), 0, "DEFAULT", 56, NULL), 0, "DEFINE", 35, NULL), 0, "DO", 57, NULL), 0, "ELSE", 49, NULL), 0, "ELSEIF", 48, NULL), 0, "ENUM", 29, NULL), 0, "EQUAL", 40, NULL), 0, "EXIT", 38, NULL), 0, "EXTENDS", 33, NULL), 0, "FAIL", 12, NULL), 0, "FALSE", 9, NULL), 0, "FOR", 59, NULL), 0, "FUNC", 31, NULL), 0, "GENERATE_IF", 44, NULL), 0, "GENERATE_ELSEIF", 45, NULL), 0, "GENERATE_ELSE", 46, NULL), 0, "HTTP", 18, NULL), 0, "I", 43, NULL), 0, "IF", 47, NULL), 0, "IMPLEMENTS", 34, NULL), 0, "IMPORT", 19, NULL), 0, "IN", 60, NULL), 0, "INFO", 18, NULL), 0, "INTERFACE", 27, NULL), 0, "IO", 18, NULL), 0, "IS", 108, NULL), 0, "ISA", 110, NULL), 0, "ISNOT", 109, NULL), 0, "ISNOTA", 111, NULL), 0, "JS", 23, NULL), 0, "MAIN", 20, NULL), 0, "MIXIN", 28, NULL), 0, "MODULE", 24, NULL), 0, "NEW", 39, NULL), 0, "NIL", 6, NULL), 0, "OK", 11, NULL), 0, "OR", 102, NULL), 0, "PARENT", 8, NULL), 0, "PROC", 32, NULL), 0, "PROCEED", 53, NULL), 0, "REPLACE", 36, NULL), 0, "RETURN", 37, NULL), 0, "SHARED", 26, NULL), 0, "SIZE", 41, NULL), 0, "SWITCH", 54, NULL), 0, "SYS", 18, NULL), 0, "THIS", 7, NULL), 0, "TRUE", 10, NULL), 0, "UNTIL", 58, NULL), 0, "VAR", 17, NULL), 0, "WHILE", 50, NULL), 0, "ZWT", 18, NULL);
 }
}
