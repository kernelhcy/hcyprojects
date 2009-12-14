#define INC_expreval_B 1
/*
 * FUNCTION BODIES
 */
#ifndef INC_node_B
#include "../ZUDIR/node.b.c"
#endif
#ifndef INC_scontext_B
#include "../ZUDIR/scontext.b.c"
#endif
Zbool MExprEval__FevalBool(CNode *Anode, CSContext *Actx) {
  switch (Anode->Vn_type)
  {
  case 91:
  case 92:
    {
      {
        char *Vleft;
        Vleft = MExprEval__FevalString(Anode->Vn_left, Actx);
        char *Vright;
        Vright = MExprEval__FevalString(Anode->Vn_right, Actx);
        return (((Zstrcmp(Vleft, Vright) == 0)) == ((Anode->Vn_type == 91)));
      }
        break;
    }
  case 102:
  case 101:
    {
      {
        Zbool Vleft;
        Vleft = MExprEval__FevalBool(Anode->Vn_left, Actx);
        if ((Vleft == ((Anode->Vn_type == 102))))
        {
          return (Anode->Vn_type == 102);
        }
        return MExprEval__FevalBool(Anode->Vn_right, Actx);
      }
        break;
    }
  case 11:
    {
      {
        return (Anode->Vn_int != 0);
      }
        break;
    }
  default:
    {
      {
        CNode__Ferror(Anode, Zconcat("Not supported here: ", Zenum2string(CNode__X__EType, sizeof(CNode__X__EType) / sizeof(char *), Anode->Vn_type)));
      }
        break;
    }
  }
  return 1;
}
char *MExprEval__FevalString(CNode *Anode, CSContext *Actx) {
  switch (Anode->Vn_type)
  {
  case 5:
    {
      {
        if ((Zstrcmp(Anode->Vn_string, "lang") == 0))
        {
          return CResolve_I__MgetLangName_ptr[Actx->Vgen->type](Actx->Vgen->ptr);
        }
      else if ((Zstrcmp(Anode->Vn_string, "permu") == 0))
        {
          return (*(char **)(Actx->Vgen->ptr + CResolve_I__VpermuName_off[Actx->Vgen->type]));
        }
      else
        {
          CNode__Ferror(Anode, Zconcat(Zconcat("Expected 'lang' or 'permu', found '", Anode->Vn_string), "'"));
        }
      }
        break;
    }
  case 9:
    {
      {
        return Anode->Vn_string;
      }
        break;
    }
  default:
    {
      {
        CNode__Ferror(Anode, Zconcat("Not supported here: ", Zenum2string(CNode__X__EType, sizeof(CNode__X__EType) / sizeof(char *), Anode->Vn_type)));
      }
        break;
    }
  }
  return "";
}
void Iexpreval() {
 static int done = 0;
 if (!done) {
  done = 1;
  Iscontext();
 }
}
