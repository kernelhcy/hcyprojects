#define INC_node_B 1
/*
 * FUNCTION BODIES
 */
#ifndef INC_attr_B
#include "../ZUDIR/attr.b.c"
#endif
#ifndef INC_conversion_B
#include "../ZUDIR/conversion.b.c"
#endif
#ifndef INC_error_B
#include "../ZUDIR/error.b.c"
#endif
#ifndef INC_token_B
#include "../ZUDIR/token.b.c"
#endif
#ifndef INC_pos_B
#include "../ZUDIR/pos.b.c"
#endif
#ifndef INC_scope_B
#include "../ZUDIR/scope.b.c"
#endif
#ifndef INC_symbol_B
#include "../ZUDIR/symbol.b.c"
#endif
#ifndef INC_usedfile_B
#include "../ZUDIR/usedfile.b.c"
#endif
CNode *CNode__FNEW(Zenum Atype) {
  CNode *THIS = Zalloc(sizeof(CNode));
  THIS->Vn_type = Atype;
  return THIS;
}
Zenum CNode__FresultType(CNode *THIS, CScope *Ascope) {
  switch (THIS->Vn_type)
  {
  case 11:
  case 12:
  case 7:
  case 9:
    {
      {
        return THIS->Vn_type;
      }
        break;
    }
  case 5:
    {
      {
        return CScope__FgetSymbolType(Ascope, THIS->Vn_string);
      }
        break;
    }
  case 77:
  case 78:
  case 79:
  case 81:
    {
      {
        return 7;
      }
        break;
    }
  case 80:
    {
      {
        if (((CNode__FresultType(THIS->Vn_left, Ascope) == 9) || (CNode__FresultType(THIS->Vn_right, Ascope) == 9)))
        {
          return 9;
        }
        return 7;
      }
        break;
    }
  case 91:
  case 92:
  case 93:
  case 94:
  case 95:
  case 96:
  case 101:
  case 102:
  case 97:
  case 98:
  case 99:
  case 100:
    {
      {
        return 11;
      }
        break;
    }
  }
  return 0;
}
CSymbol *CNode__FfindTopModule(CNode *THIS, CScope *Ascope) {
  CSymbol *Vsym;
  Vsym = NULL;
  if ((THIS->Vn_type == 5))
  {
    Vsym = CScope__FgetSymbol(Ascope, THIS->Vn_string);
  }
else if ((THIS->Vn_type == 105))
  {
    Vsym = CNode__FfindTopModule(THIS->Vn_left, Ascope);
    if ((Vsym != NULL))
    {
      Vsym = CSymbol__FfindMember(Vsym, THIS->Vn_string);
    }
  }
  if (((Vsym == NULL) || (((Vsym->Vtype != 19) && (Vsym->Vtype != 20)))))
  {
    return NULL;
  }
  return Vsym;
}
void CNode__Ferror(CNode *THIS, char *Amsg) {
  MError__Freport__1(Amsg, THIS->Vn_start);
}
char *CNode__FtoString(CNode *THIS) {
  return CNode__FtoString__1(THIS, "", 0, 0);
}
char *CNode__FtypeString(CNode *THIS) {
  return Zconcat(Zconcat("type: ", Zenum2string(CNode__X__EType, sizeof(CNode__X__EType) / sizeof(char *), THIS->Vn_type)), "\n");
}
char *CNode__FtoString__1(CNode *THIS, char *Aindent, Zbool Arecurse, Zbool AdoNext) {
  char *Vs;
  Vs = Zconcat(Aindent, CNode__FtypeString(THIS));
  if (((Zstrcmp(Aindent, "") == 0) && (THIS->Vn_start != NULL)))
  {
    Vs = Zconcat(Vs, Zconcat(Zconcat("start: ", CPos__FtoString(THIS->Vn_start)), "\n"));
  }
  if ((THIS->Vn_string != NULL))
  {
    Vs = Zconcat(Vs, Zconcat(Zconcat(Zconcat(Aindent, "name: "), THIS->Vn_string), "\n"));
  }
  if ((THIS->Vn_int != 0))
  {
    Vs = Zconcat(Vs, Zconcat(Zconcat(Zconcat(Aindent, "value: "), Zint2string(THIS->Vn_int)), "\n"));
  }
  if ((THIS->Vn_undefined != 0))
  {
    Vs = Zconcat(Vs, Zconcat(Zconcat(Zconcat(Aindent, "undefined: "), Zint2string(THIS->Vn_undefined)), "\n"));
  }
  if ((THIS->Vn_symbol != NULL))
  {
    Vs = Zconcat(Vs, Zconcat(Zconcat(Zconcat(Aindent, "symbol: "), CSymbol__FtoString(THIS->Vn_symbol)), "\n"));
  }
  if ((THIS->Vn_conversion != 0))
  {
    Vs = Zconcat(Vs, Zconcat(Zconcat(Zconcat(Aindent, "conversion: "), Zenum2string(EConversion, sizeof(EConversion) / sizeof(char *), THIS->Vn_conversion)), "\n"));
  }
  if ((THIS->Vn_left != NULL))
  {
    if (Arecurse)
    {
      Vs = Zconcat(Vs, Zconcat(Zconcat(Aindent, "left:\n"), CNode__FtoString__1(THIS->Vn_left, Zconcat(Aindent, "|  "), Arecurse, AdoNext)));
    }
  else
    {
      Vs = Zconcat(Vs, Zconcat(Zconcat(Aindent, "left: "), CNode__FtypeString(THIS->Vn_left)));
    }
  }
  if ((THIS->Vn_right != NULL))
  {
    if (Arecurse)
    {
      Vs = Zconcat(Vs, Zconcat(Zconcat(Aindent, "right:\n"), CNode__FtoString__1(THIS->Vn_right, Zconcat(Aindent, "|  "), Arecurse, AdoNext)));
    }
  else
    {
      Vs = Zconcat(Vs, Zconcat(Zconcat(Aindent, "right: "), CNode__FtypeString(THIS->Vn_right)));
    }
  }
  if ((AdoNext && (THIS->Vn_next != NULL)))
  {
    return Zconcat(Zconcat(Zconcat(Vs, Aindent), "next:-------------------------\n"), CNode__FtoString__1(THIS->Vn_next, Aindent, Arecurse, AdoNext));
  }
  return Vs;
}
void CNode__FcheckTypeName(CNode *THIS, char *Awhat) {
  if (((THIS->Vn_string[0] < 65) || (THIS->Vn_string[0] > 90)))
  {
    CNode__Ferror(THIS, Zconcat(Awhat, " name must start with upper case letter"));
  }
}
void CNode__FcheckItemName(CNode *THIS, char *Awhat) {
  if (((((THIS->Vn_string[0] < 97) || (THIS->Vn_string[0] > 122))) && (THIS->Vn_string[0] != 95)))
  {
    CNode__Ferror(THIS, Zconcat(Awhat, " name must start with lower case letter or '_'"));
  }
}
Zenum CNode__X__Fname2Type(char *Aname) {
  if ((Zstrcmp(Aname, "string") == 0))
  {
    return 9;
  }
  if ((Zstrcmp(Aname, "int") == 0))
  {
    return 7;
  }
  if ((Zstrcmp(Aname, "bool") == 0))
  {
    return 11;
  }
  if ((Zstrcmp(Aname, "status") == 0))
  {
    return 12;
  }
  if ((Zstrcmp(Aname, "list") == 0))
  {
    return 13;
  }
  if ((Zstrcmp(Aname, "dict") == 0))
  {
    return 14;
  }
  return 0;
}
Zbool CNode__X__FisPointerType(Zenum Atype) {
  switch (Atype)
  {
  case 2:
  case 21:
  case 24:
  case 25:
  case 37:
  case 36:
  case 9:
  case 13:
  case 14:
    {
      {
        return 1;
      }
        break;
    }
  }
  return 0;
}
Zbool CNode__X__FisMethodType(Zenum Atype) {
  return (((((Atype == 111) || (Atype == 35)) || (Atype == 34)) || (Atype == 33)) || (Atype == 32));
}
Zbool CNode__X__FisTypeWithArgs(Zenum Atype) {
  switch (Atype)
  {
  case 32:
  case 33:
  case 35:
  case 34:
  case 111:
    {
      {
        return 1;
      }
        break;
    }
  }
  return 0;
}
void Inode() {
 static int done = 0;
 if (!done) {
  done = 1;
  Iscope();
  Isymbol();
  Iusedfile();
 }
}
