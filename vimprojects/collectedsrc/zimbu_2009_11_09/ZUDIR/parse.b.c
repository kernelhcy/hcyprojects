#define INC_parse_B 1
/*
 * FUNCTION BODIES
 */
#ifndef INC_attr_B
#include "../ZUDIR/attr.b.c"
#endif
#ifndef INC_error_B
#include "../ZUDIR/error.b.c"
#endif
#ifndef INC_input_B
#include "../ZUDIR/input.b.c"
#endif
#ifndef INC_node_B
#include "../ZUDIR/node.b.c"
#endif
#ifndef INC_pos_B
#include "../ZUDIR/pos.b.c"
#endif
#ifndef INC_scope_B
#include "../ZUDIR/scope.b.c"
#endif
#ifndef INC_token_B
#include "../ZUDIR/token.b.c"
#endif
#ifndef INC_tokenize_B
#include "../ZUDIR/tokenize.b.c"
#endif
#ifndef INC_usedfile_B
#include "../ZUDIR/usedfile.b.c"
#endif
CScope *MParse__FparseFile(char *AfileName, char *Aindent, CUsedFile *AusedFile) {
  Zbool VisMainFile;
  VisMainFile = ((AusedFile != NULL) && AusedFile->VisMainFile);
  FILE *VinFile;
  VinFile = fopen(AfileName, "r");
  if ((VinFile == NULL))
  {
    return NULL;
  }
  CInput *Vin;
  Vin = CInput__FNEW(VinFile, AfileName);
  MError__FverboseMsg(Zconcat(Zconcat(Aindent, AfileName), ": Parsing...\n"));
  CNode *Vnode;
  Vnode = MParse__FparseInput(Vin, VisMainFile);
  fclose(VinFile);
  CScope *VtopScope;
  VtopScope = CScope__X__FnewScope(NULL, 1);
  VtopScope->Vpass = 1;
  VtopScope->VimportIndent = Aindent;
  VtopScope->VtopNode = Vnode;
  VtopScope->VzimbuFile = AusedFile->VzimbuFile;
  VtopScope->VusedIdKeywords = Vin->VusedIdKeywords;
  VtopScope->VusedItems = ZnewDict(1);
  if ((MError__Vdebug && VisMainFile))
  {
    fputs(CNode__FtoString__1(Vnode, "", 1, 1), stdout);
  }
  return VtopScope;
}
CNode *MParse__FparseInput(CInput *Ain, Zbool AallowMain) {
  CNode *Vtop_node;
  Vtop_node = CNode__FNEW(0);
  CNode *Vend_node;
  Vend_node = Vtop_node;
  CToken *Vtoken;
  Vtoken = NULL;
  Zbool VhadMain;
  VhadMain = 0;
  Zbool VhadBlockItem;
  VhadBlockItem = 0;
  MParse__FskipSep(Ain);
  while (1)
  {
    Vtoken = CInput__FgetToken(Ain);
    if ((Vtoken->Vtype != 19))
    {
      CInput__FpushToken(Ain, Vtoken);
      break;
    }
    char *Vspec;
    Vspec = NULL;
    CToken *Vnext;
    Vnext = MParse__FtokenAfterSep(Ain);
    if ((Vnext->Vtype == 65))
    {
      MParse__FcheckNoSep(Ain);
      Vtoken = CInput__FgetToken(Ain);
      MParse__FcheckNoSep(Ain);
      Vtoken = CInput__FgetToken(Ain);
      if ((Vtoken->Vtype != 18))
      {
        CToken__Ferror(Vtoken, "IMPORT. must be followed by a name");
      }
    else
      {
        Vspec = Vtoken->Vvalue;
      }
    }
    MParse__FexpectSep(Ain);
    Vtoken = CInput__FgetToken(Ain);
    if ((Vtoken->Vtype != 13))
    {
      CToken__Ferror(Vtoken, "IMPORT must be followed by a string");
      CInput__FpushToken(Ain, Vtoken);
    }
  else
    {
      Vend_node = MParse__FnewNode(Vend_node, Vtoken);
      Vend_node->Vn_type = 38;
      Vend_node->Vn_string = Vtoken->Vvalue;
      if ((Vspec != NULL))
      {
        Vend_node->Vn_left = CNode__FNEW(5);
        Vend_node->Vn_left->Vn_string = Vspec;
      }
      MParse__FexpectNewLine(Ain);
    }
  }
  while (1)
  {
    Vtoken = CInput__FgetToken(Ain);
    if ((Vtoken->Vtype == 1))
    {
      break;
    }
    if ((Vtoken->Vtype == 20))
    {
      if (!(AallowMain))
      {
        CToken__Ferror(Vtoken, "MAIN not allowed in imported file");
      }
    else if (VhadMain)
      {
        CToken__Ferror(Vtoken, "Duplicate MAIN");
      }
      VhadMain = 1;
      if (((CInput__FgetToken(Ain)->Vtype != 66) || (CInput__FgetToken(Ain)->Vtype != 67)))
      {
        CToken__Ferror(Vtoken, "Expected ()");
      }
      MParse__FexpectNewLine(Ain);
      Vend_node = MParse__FnewNode(Vend_node, Vtoken);
      Vend_node->Vn_type = 39;
      Vend_node->Vn_left = MParse__FparseBlock(Ain, 0);
    }
  else
    {
      if ((!(AallowMain) && VhadBlockItem))
      {
        CToken__Ferror(Vtoken, "Only one toplevel item allowed");
      }
      VhadBlockItem = 1;
      CInput__FpushToken(Ain, Vtoken);
      CNode *Vnode;
      Vnode = MParse__FparseBlockItem(Ain, Vend_node, ((1 + ((AallowMain) ? (0) : (2))) + 4));
      if ((Vnode == NULL))
      {
        Vtoken = CInput__FgetToken(Ain);
        if ((Vtoken->Vtype == 69))
        {
          CToken__Ferror(Vtoken, "unexpected }");
        }
      else
        {
          CToken__FerrorNotAllowed(Vtoken);
        }
      }
    else
      {
        Vend_node = Vnode;
      }
    }
  }
  Vend_node->Vn_next = NULL;
  return Vtop_node->Vn_next;
}
CNode *MParse__FnewNode(CNode *Aend_node, CToken *Atoken) {
  CNode *Vnode;
  Vnode = CNode__FNEW(0);
  Aend_node->Vn_next = Vnode;
  Vnode->Vn_start = CPos__Fcopy(Atoken->VstartPos);
  return Vnode;
}
CNode *MParse__FparseBlock(CInput *Ain, Zbits AblockType) {
  CNode *Vtop_node;
  Vtop_node = CNode__FNEW(0);
  CNode *Vend_node;
  Vend_node = Vtop_node;
  while (1)
  {
    CNode *Vn;
    Vn = MParse__FparseBlockItem(Ain, Vend_node, AblockType);
    if ((Vn == NULL))
    {
      break;
    }
    Vend_node = Vn;
  }
  if (!((((AblockType) & 4))))
  {
    CToken *Vtoken;
    Vtoken = CInput__FgetToken(Ain);
    if ((Vtoken->Vtype != 69))
    {
      CToken__Ferror(Vtoken, "Syntax error");
    }
  else
    {
      MParse__FexpectNewLine(Ain);
    }
  }
  Vend_node->Vn_next = NULL;
  return Vtop_node->Vn_next;
}
CNode *MParse__FparseBlockItem(CInput *Ain, CNode *Astart_node, Zbits AblockType) {
  CNode *Vend_node;
  Vend_node = Astart_node;
  CToken *Vtoken;
  Vtoken = CInput__FgetToken(Ain);
  switch (Vtoken->Vtype)
  {
  case 1:
    {
      {
        MParse__Ferror("unexpected EOF", Ain);
        return NULL;
      }
        break;
    }
  case 3:
  case 2:
  case 4:
    {
      {
        return Vend_node;
      }
        break;
    }
  case 69:
    {
      {
        CInput__FpushToken(Ain, Vtoken);
        return NULL;
      }
        break;
    }
  case 13:
    {
      {
        CToken__Ferror(Vtoken, Zconcat(Zconcat("unexpected string with value '", Vtoken->Vvalue), "'"));
        return Vend_node;
      }
        break;
    }
  case 21:
    {
      {
        CNode *Vnode;
        Vnode = MParse__FcopyCode(Ain);
        Vend_node->Vn_next = Vnode;
        return Vnode;
      }
        break;
    }
  case 24:
    {
      {
        Vend_node = MParse__FnewNode(Vend_node, Vtoken);
        Vend_node->Vn_type = 19;
        MParse__FexpectSep(Ain);
        Vtoken = CInput__FgetToken(Ain);
        if ((Vtoken->Vtype != 18))
        {
          CToken__Ferror(Vtoken, "MODULE must be followed by a name");
        }
        Vend_node->Vn_string = Vtoken->Vvalue;
        MParse__FexpectNewLine(Ain);
        Vend_node->Vn_left = MParse__FparseBlock(Ain, 1);
        return Vend_node;
      }
        break;
    }
  case 36:
  case 35:
    {
      {
        Zbbits Va;
        Va = 0;
        if ((Vtoken->Vtype == 35))
        {
          Va = ((Va) & -9) | ((1) << 3);
          MParse__FskipSep(Ain);
        }
      else
        {
          Va = ((Va) & -17) | ((1) << 4);
          MParse__FskipSep(Ain);
        }
        CToken *Vnext;
        Vnext = CInput__FgetToken(Ain);
        if (((((Vnext->Vtype == 39) || (Vnext->Vtype == 40)) || (Vnext->Vtype == 31)) || (Vnext->Vtype == 32)))
        {
          return MParse__FparseMethod(Vnext, Va, Vend_node, Ain, AblockType);
        }
        if (((Vnext->Vtype == 35) || (Vnext->Vtype == 36)))
        {
          if ((Vtoken->Vtype == Vnext->Vtype))
          {
            CToken__Ferror(Vnext, Zconcat("Duplicate ", Vtoken->Vvalue));
          }
        else
          {
            CToken__Ferror(Vnext, "Cannot have both DEFINE and REPLACE");
          }
        }
      else
        {
          CToken__Ferror(Vtoken, "Expected FUNC, PROC, NEW or EQUAL");
        }
        return Vend_node;
      }
        break;
    }
  case 25:
  case 27:
    {
      {
        return MParse__FparseClass(Vtoken, 0, Vend_node, Ain);
      }
        break;
    }
  case 26:
    {
      {
        Vend_node = MParse__FnewNode(Vend_node, Vtoken);
        Vend_node->Vn_type = 22;
        MParse__FexpectNewLine(Ain);
        Vend_node->Vn_left = MParse__FparseBlock(Ain, 1);
        Vtoken = CInput__FgetToken(Ain);
        if ((Vtoken->Vtype != 69))
        {
          CToken__Ferror(Vtoken, "No class items allowed after SHARED section");
        }
        CInput__FpushToken(Ain, Vtoken);
        return Vend_node;
      }
        break;
    }
  case 40:
  case 39:
  case 32:
  case 31:
    {
      {
        return MParse__FparseMethod(Vtoken, 0, Vend_node, Ain, AblockType);
      }
        break;
    }
  case 30:
    {
      {
        Vend_node = MParse__FnewNode(Vend_node, Vtoken);
        Vend_node->Vn_type = 28;
        MParse__FexpectSep(Ain);
        Vtoken = CInput__FgetToken(Ain);
        if ((Vtoken->Vtype != 18))
        {
          CToken__Ferror(Vtoken, "BITS must be followed by a name");
        }
        Vend_node->Vn_string = Vtoken->Vvalue;
        MParse__FexpectNewLine(Ain);
        Vend_node->Vn_left = MParse__FparseBlock(Ain, 1);
        return Vend_node;
      }
        break;
    }
  case 29:
    {
      {
        Vend_node = MParse__FnewNode(Vend_node, Vtoken);
        Vend_node->Vn_type = 31;
        MParse__FexpectSep(Ain);
        Vtoken = CInput__FgetToken(Ain);
        if ((Vtoken->Vtype != 18))
        {
          CToken__Ferror(Vtoken, "ENUM must be followed by a name");
        }
        Vend_node->Vn_string = Vtoken->Vvalue;
        Vend_node->Vn_left = NULL;
        CNode *Vnode;
        Vnode = Vend_node;
        MParse__FexpectNewLine(Ain);
        while (1)
        {
          Vtoken = CInput__FgetToken(Ain);
          if ((Vtoken->Vtype == 69))
          {
            MParse__FexpectNewLine(Ain);
            break;
          }
          if ((Vtoken->Vtype != 18))
          {
            CToken__Ferror(Vtoken, "unexpected token in ENUM");
            break;
          }
          CToken *Vnext;
          Vnext = MParse__FtokenAfterSep(Ain);
          if ((Vnext->Vtype == 69))
          {
            MParse__FexpectNewLine(Ain);
          }
        else
          {
            MParse__FexpectSep(Ain);
          }
          CNode *Ven;
          Ven = CNode__FNEW(30);
          Vnode->Vn_left = Ven;
          Vnode = Ven;
          Vnode->Vn_string = Vtoken->Vvalue;
          Vnode->Vn_start = CPos__Fcopy(Vtoken->VstartPos);
          Vnode->Vn_left = NULL;
        }
        return Vend_node;
      }
        break;
    }
  case 75:
  case 73:
    {
      {
        MParse__FcheckNoSep(Ain);
        CNode *Vone_node;
        Vone_node = MParse__FparseDotName(Ain);
        if ((Vone_node->Vn_type == 107))
        {
          CToken__Ferror(Vtoken, Zconcat(Zconcat("Cannot use ", Vtoken->Vvalue), " here"));
        }
        Vend_node = MParse__FnewNode(Vend_node, Vtoken);
        if ((Vtoken->Vtype == 75))
        {
          Vend_node->Vn_type = 69;
        }
      else
        {
          Vend_node->Vn_type = 70;
        }
        Vend_node->Vn_left = Vone_node;
        MParse__FexpectLineSep(Ain);
        return Vend_node;
      }
        break;
    }
  case 18:
  case 16:
  case 17:
  case 7:
  case 8:
    {
      {
        if ((((AblockType) & 2)))
        {
          CToken__FerrorNotAllowed(Vtoken);
        }
        CNode *Vone_node;
        Vone_node = NULL;
        if ((((Vtoken->Vtype == 18) || (Vtoken->Vtype == 7)) || (Vtoken->Vtype == 8)))
        {
          CInput__FpushToken(Ain, Vtoken);
          Vone_node = MParse__FparseDotName(Ain);
        }
      else
        {
          Vone_node = CNode__FNEW(((Vtoken->Vtype == 17)) ? (18) : (17));
          Vone_node->Vn_string = Vtoken->Vvalue;
          Vone_node->Vn_start = Vtoken->VstartPos;
        }
        if ((Vone_node->Vn_type == 107))
        {
          if ((((AblockType) & 1)))
          {
            CToken__Ferror(Vtoken, "cannot call function in this scope");
          }
          Vend_node = MParse__FnewNode(Vend_node, Vtoken);
          Vend_node->Vn_type = 62;
          Vend_node->Vn_left = Vone_node;
          MParse__FexpectLineSep(Ain);
        }
      else
        {
          CToken *VafterSep;
          VafterSep = MParse__FtokenAfterSep(Ain);
          if (((VafterSep->Vtype != 75) && (VafterSep->Vtype != 73)))
          {
            MParse__FexpectSep(Ain);
          }
          Vtoken = CInput__FgetToken(Ain);
          if (((((Vtoken->Vtype == 86) || (Vtoken->Vtype == 87)) || (Vtoken->Vtype == 88)) || (Vtoken->Vtype == 96)))
          {
            if ((((AblockType) & 1)))
            {
              CToken__Ferror(Vtoken, "cannot do assignment in this scope");
            }
            Vend_node = MParse__FnewNode(Vend_node, Vtoken);
            switch (Vtoken->Vtype)
            {
            case 86:
              {
                {
                  Vend_node->Vn_type = 54;
                }
                  break;
              }
            case 87:
              {
                {
                  Vend_node->Vn_type = 55;
                }
                  break;
              }
            case 88:
              {
                {
                  Vend_node->Vn_type = 56;
                }
                  break;
              }
            case 96:
              {
                {
                  Vend_node->Vn_type = 57;
                }
                  break;
              }
            }
            Vend_node->Vn_left = Vone_node;
            MParse__FexpectSep(Ain);
            Vend_node->Vn_right = MParse__FparseExpr(Ain);
            MParse__FexpectLineSep(Ain);
          }
        else if (((VafterSep->Vtype == 75) || (VafterSep->Vtype == 73)))
          {
            if (((Vtoken->Vtype != 75) && (Vtoken->Vtype != 73)))
            {
              CToken__Ferror(Vtoken, "superfluous white space");
              Vtoken = CInput__FgetToken(Ain);
            }
            if ((((AblockType) & 1)))
            {
              CToken__Ferror(Vtoken, "cannot do in/decrement in this scope");
            }
          else if (((Vone_node->Vn_type == 18) || (Vone_node->Vn_type == 17)))
            {
              CToken__Ferror(Vtoken, "cannot in/decrement ANY or VAR");
            }
            Vend_node = MParse__FnewNode(Vend_node, Vtoken);
            if ((Vtoken->Vtype == 75))
            {
              Vend_node->Vn_type = 69;
            }
          else
            {
              Vend_node->Vn_type = 70;
            }
            Vend_node->Vn_left = Vone_node;
            MParse__FexpectLineSep(Ain);
          }
        else if ((Vtoken->Vtype != 18))
          {
            CInput__FpushToken(Ain, Vtoken);
            if ((Vone_node->Vn_type == 5))
            {
              CToken__Ferror(Vtoken, Zconcat("name without operation: ", Vone_node->Vn_string));
            }
          else
            {
              CToken__Ferror(Vtoken, "Syntax error");
            }
          }
        else
          {
            Vend_node = MParse__FnewNode(Vend_node, Vtoken);
            Vend_node->Vn_type = 66;
            Vend_node->Vn_left = Vone_node;
            Vend_node->Vn_string = Vtoken->Vvalue;
            Vtoken = MParse__FtokenAfterSep(Ain);
            if ((Vtoken->Vtype == 86))
            {
              MParse__FexpectSep(Ain);
              Vtoken = CInput__FgetToken(Ain);
              MParse__FexpectSep(Ain);
              Vend_node->Vn_right = MParse__FparseExpr(Ain);
            }
          else
            {
              Vend_node->Vn_right = NULL;
            }
            MParse__FexpectNewLine(Ain);
          }
        }
        return Vend_node;
      }
        break;
    }
  case 44:
    {
      {
        Vend_node = MParse__FnewNode(Vend_node, Vtoken);
        Vend_node->Vn_type = 42;
        CNode *Vgen_node;
        Vgen_node = CNode__FNEW(42);
        Vend_node->Vn_left = Vgen_node;
        MParse__FexpectSep(Ain);
        Vgen_node->Vn_cond = MParse__FparseExpr(Ain);
        MParse__FexpectLineSep(Ain);
        Zbool VhadElse;
        VhadElse = 0;
        while (1)
        {
          Vgen_node = MParse__FnewNode(Vgen_node, Vtoken);
          Vgen_node->Vn_type = 58;
          Vgen_node->Vn_left = MParse__FparseBlock(Ain, 4);
          Vtoken = CInput__FgetToken(Ain);
          if ((Vtoken->Vtype == 45))
          {
            Vgen_node = MParse__FnewNode(Vgen_node, Vtoken);
            Vgen_node->Vn_type = 43;
            MParse__FexpectSep(Ain);
            Vgen_node->Vn_cond = MParse__FparseExpr(Ain);
            MParse__FexpectLineSep(Ain);
          }
        else if ((Vtoken->Vtype == 46))
          {
            if (VhadElse)
            {
              CToken__Ferror(Vtoken, "already had a GENERATE_ELSE");
            }
            Vgen_node = MParse__FnewNode(Vgen_node, Vtoken);
            Vgen_node->Vn_type = 44;
            MParse__FexpectLineSep(Ain);
            VhadElse = 1;
          }
        else if ((Vtoken->Vtype == 69))
          {
            MParse__FexpectNewLine(Ain);
            break;
          }
        else
          {
            CToken__Ferror(Vtoken, "Syntax error");
            CInput__FpushToken(Ain, Vtoken);
            break;
          }
        }
        return Vend_node;
      }
        break;
    }
  case 45:
    {
      {
        CInput__FpushToken(Ain, Vtoken);
        return NULL;
      }
        break;
    }
  case 46:
    {
      {
        CInput__FpushToken(Ain, Vtoken);
        return NULL;
      }
        break;
    }
  default:
    break;
  }
  if ((((AblockType) & 1)))
  {
    CToken__FerrorNotAllowed(Vtoken);
  }
  switch (Vtoken->Vtype)
  {
  case 47:
    {
      {
        Vend_node = MParse__FnewNode(Vend_node, Vtoken);
        Vend_node->Vn_type = 45;
        MParse__FexpectSep(Ain);
        Vend_node->Vn_cond = MParse__FparseExpr(Ain);
        MParse__FexpectLineSep(Ain);
        Zbool VhadElse;
        VhadElse = 0;
        while (1)
        {
          Vend_node = MParse__FnewNode(Vend_node, Vtoken);
          Vend_node->Vn_type = 58;
          Vend_node->Vn_nodeType = 45;
          Vend_node->Vn_left = MParse__FparseBlock(Ain, 4);
          Vtoken = CInput__FgetToken(Ain);
          if ((Vtoken->Vtype == 48))
          {
            Vend_node = MParse__FnewNode(Vend_node, Vtoken);
            Vend_node->Vn_type = 47;
            MParse__FexpectSep(Ain);
            Vend_node->Vn_cond = MParse__FparseExpr(Ain);
            MParse__FexpectLineSep(Ain);
          }
        else if ((Vtoken->Vtype == 49))
          {
            if (VhadElse)
            {
              CToken__Ferror(Vtoken, "already had an ELSE");
            }
            Vend_node = MParse__FnewNode(Vend_node, Vtoken);
            Vend_node->Vn_type = 46;
            MParse__FexpectLineSep(Ain);
            VhadElse = 1;
          }
        else if ((Vtoken->Vtype == 69))
          {
            MParse__FexpectNewLine(Ain);
            break;
          }
        else
          {
            CToken__Ferror(Vtoken, "Syntax error");
            CInput__FpushToken(Ain, Vtoken);
            break;
          }
        }
      }
        break;
    }
  case 48:
    {
      {
        CInput__FpushToken(Ain, Vtoken);
        return NULL;
      }
        break;
    }
  case 49:
    {
      {
        CInput__FpushToken(Ain, Vtoken);
        return NULL;
      }
        break;
    }
  case 50:
    {
      {
        Vend_node = MParse__FnewNode(Vend_node, Vtoken);
        Vend_node->Vn_type = 48;
        MParse__FexpectSep(Ain);
        Vend_node->Vn_cond = MParse__FparseExpr(Ain);
        MParse__FexpectLineSep(Ain);
        Vend_node->Vn_right = MParse__FparseBlock(Ain, 0);
      }
        break;
    }
  case 57:
    {
      {
        MParse__FexpectLineSep(Ain);
        Vend_node = MParse__FnewNode(Vend_node, Vtoken);
        Vend_node->Vn_type = 67;
        Vend_node = MParse__FnewNode(Vend_node, Vtoken);
        Vend_node->Vn_type = 58;
        Vend_node->Vn_nodeType = 67;
        Vend_node->Vn_left = MParse__FparseBlock(Ain, 4);
        Vtoken = CInput__FgetToken(Ain);
        if ((Vtoken->Vtype != 58))
        {
          CToken__Ferror(Vtoken, "Missing UNTIL");
        }
        Vend_node = MParse__FnewNode(Vend_node, Vtoken);
        Vend_node->Vn_type = 68;
        MParse__FexpectSep(Ain);
        Vend_node->Vn_cond = MParse__FparseExpr(Ain);
        MParse__FexpectNewLine(Ain);
      }
        break;
    }
  case 58:
    {
      {
        CInput__FpushToken(Ain, Vtoken);
        return NULL;
      }
        break;
    }
  case 59:
    {
      {
        Vend_node = MParse__FnewNode(Vend_node, Vtoken);
        Vend_node->Vn_type = 49;
        MParse__FexpectSep(Ain);
        Vend_node->Vn_left = MParse__FparseDotName(Ain);
        MParse__FexpectSep(Ain);
        Vtoken = CInput__FgetToken(Ain);
        if ((Vtoken->Vtype != 60))
        {
          CToken__Ferror(Vtoken, "Missing IN");
        }
        MParse__FexpectSep(Ain);
        Vend_node->Vn_cond = MParse__FparseExpr(Ain);
        MParse__FexpectLineSep(Ain);
        Vend_node->Vn_right = MParse__FparseBlock(Ain, 0);
      }
        break;
    }
  case 37:
    {
      {
        Vend_node = MParse__FnewNode(Vend_node, Vtoken);
        Vend_node->Vn_type = 71;
        CToken *Vnext;
        Vnext = CInput__FgetToken(Ain);
        CInput__FpushToken(Ain, Vnext);
        MParse__FskipSep(Ain);
        Vend_node->Vn_left = MParse__FparseExpr(Ain);
        if (((Vend_node->Vn_left->Vn_type != 0) || (Vnext->Vtype != 3)))
        {
          MParse__FexpectLineSep(Ain);
        }
      }
        break;
    }
  case 38:
    {
      {
        Vend_node = MParse__FnewNode(Vend_node, Vtoken);
        Vend_node->Vn_type = 72;
        MParse__FskipSep(Ain);
        Vend_node->Vn_left = MParse__FparseExpr(Ain);
        MParse__FexpectLineSep(Ain);
      }
        break;
    }
  case 51:
    {
      {
        Vend_node = MParse__FnewNode(Vend_node, Vtoken);
        Vend_node->Vn_type = 51;
        MParse__FexpectLineSep(Ain);
      }
        break;
    }
  case 53:
    {
      {
        CInput__FpushToken(Ain, Vtoken);
        return NULL;
      }
        break;
    }
  case 52:
    {
      {
        Vend_node = MParse__FnewNode(Vend_node, Vtoken);
        Vend_node->Vn_type = 52;
        MParse__FexpectLineSep(Ain);
      }
        break;
    }
  case 54:
    {
      {
        Vend_node = MParse__FnewNode(Vend_node, Vtoken);
        Vend_node->Vn_type = 63;
        MParse__FskipSep(Ain);
        Vend_node->Vn_cond = MParse__FparseExpr(Ain);
        MParse__FexpectLineSep(Ain);
        CNode *VswitchNode;
        VswitchNode = CNode__FNEW(0);
        CNode *Vnnode;
        Vnnode = VswitchNode;
        Zbool VhadCase;
        VhadCase = 0;
        Zbool VhadDefault;
        VhadDefault = 0;
        while (1)
        {
          Vtoken = CInput__FgetToken(Ain);
          if ((Vtoken->Vtype == 55))
          {
            VhadCase = 1;
            Vnnode = MParse__FnewNode(Vnnode, Vtoken);
            Vnnode->Vn_type = 64;
            MParse__FskipSep(Ain);
            Vnnode->Vn_left = MParse__FparseExpr(Ain);
            MParse__FexpectLineSep(Ain);
          }
        else if ((Vtoken->Vtype == 56))
          {
            if (VhadDefault)
            {
              CToken__Ferror(Vtoken, "Duplicate DEFAULT");
            }
            VhadDefault = 1;
            Vnnode = MParse__FnewNode(Vnnode, Vtoken);
            Vnnode->Vn_type = 65;
            MParse__FexpectLineSep(Ain);
          }
        else if (((Vtoken->Vtype == 69) || (Vtoken->Vtype == 1)))
          {
            break;
          }
        else
          {
            if ((!(VhadCase) && !(VhadDefault)))
            {
              CToken__Ferror(Vtoken, "Unexpected item in SWITCH");
            }
            CInput__FpushToken(Ain, Vtoken);
            Vnnode = MParse__FnewNode(Vnnode, Vtoken);
            Vnnode->Vn_type = 58;
            Vnnode->Vn_nodeType = 63;
            Vnnode->Vn_left = MParse__FparseBlock(Ain, 4);
            Vtoken = MParse__FtokenAfterSep(Ain);
            if ((Vtoken->Vtype == 53))
            {
              Vtoken = CInput__FgetToken(Ain);
              MParse__FexpectLineSep(Ain);
              Vnnode = MParse__FnewNode(Vnnode, Vtoken);
              Vnnode->Vn_type = 53;
            }
          }
        }
        Vend_node->Vn_right = VswitchNode->Vn_next;
      }
        break;
    }
  case 55:
    {
      {
        CInput__FpushToken(Ain, Vtoken);
        return NULL;
      }
        break;
    }
  case 56:
    {
      {
        CInput__FpushToken(Ain, Vtoken);
        return NULL;
      }
        break;
    }
  case 68:
    {
      {
        MParse__FexpectLineSep(Ain);
        Vend_node = MParse__FnewNode(Vend_node, Vtoken);
        Vend_node->Vn_type = 58;
        Vend_node->Vn_nodeType = 58;
        Vend_node->Vn_left = MParse__FparseBlock(Ain, 0);
      }
        break;
    }
  case 62:
    {
      {
        CToken__Ferror(Vtoken, "unexpected ;");
      }
        break;
    }
  default:
    {
      {
        CToken__Ferror(Vtoken, Zconcat(Zconcat("unexpected '", Vtoken->Vvalue), "'"));
      }
        break;
    }
  }
  return Vend_node;
}
CNode *MParse__FparseClass(CToken *AclassToken, Zbbits Aattr, CNode *Anode_in, CInput *Ain) {
  CNode *Vend_node;
  Vend_node = MParse__FnewNode(Anode_in, AclassToken);
  Zbool VisInterface;
  VisInterface = (AclassToken->Vtype == 27);
  Vend_node->Vn_type = (VisInterface) ? (23) : (21);
  Vend_node->Vn_attr = Aattr;
  CToken *Vtoken;
  Vtoken = CInput__FgetToken(Ain);
  if ((Vtoken->Vtype != 65))
  {
    CInput__FpushToken(Ain, Vtoken);
  }
else
  {
    Vtoken = CInput__FgetToken(Ain);
    if ((Vtoken->Vtype != 18))
    {
      CToken__Ferror(Vtoken, Zconcat("Unexpected item ", Zenum2string(CToken__EType, sizeof(CToken__EType) / sizeof(char *), Vtoken->Vtype)));
    }
  else if ((Zstrcmp(Vtoken->Vvalue, "final") == 0))
    {
      Vend_node->Vn_attr = ((Vend_node->Vn_attr) & -5) | ((1) << 2);
    }
  else if ((Zstrcmp(Vtoken->Vvalue, "abstract") == 0))
    {
      if ((AclassToken->Vtype == 27))
      {
        CToken__Ferror(Vtoken, "abstract attribute not supported for interface");
      }
      Vend_node->Vn_attr = ((Vend_node->Vn_attr) & -2) | ((1));
    }
  else
    {
      CToken__Ferror(Vtoken, Zconcat(Vtoken->Vvalue, " attribute not supported"));
    }
  }
  if (VisInterface)
  {
    Vend_node->Vn_attr = ((Vend_node->Vn_attr) & -2) | ((1));
  }
  MParse__FexpectSep(Ain);
  Vtoken = CInput__FgetToken(Ain);
  if ((Vtoken->Vtype != 18))
  {
    CToken__Ferror(Vtoken, "CLASS must be followed by a name");
  }
  Vend_node->Vn_string = Vtoken->Vvalue;
  Vtoken = MParse__FtokenAfterSep(Ain);
  if ((Vtoken->Vtype == 33))
  {
    MParse__FexpectSep(Ain);
    Vtoken = CInput__FgetToken(Ain);
    MParse__FexpectSep(Ain);
    Vend_node->Vn_cond = MParse__FparseExpr(Ain);
    Vtoken = MParse__FtokenAfterSep(Ain);
  }
  if ((Vtoken->Vtype == 34))
  {
    if (VisInterface)
    {
      CToken__Ferror(Vtoken, "cannot use IMPLEMENTS on an interface");
    }
    MParse__FexpectSep(Ain);
    Vtoken = CInput__FgetToken(Ain);
    MParse__FexpectSep(Ain);
    Vend_node->Vn_right = MParse__FparseComma(Ain, 0);
  }
  MParse__FexpectNewLine(Ain);
  Vend_node->Vn_left = MParse__FparseBlock(Ain, (1 + ((VisInterface) ? (8) : (0))));
  return Vend_node;
}
CNode *MParse__FparseMethod(CToken *AfirstToken, Zbbits Aattr, CNode *Anode_in, CInput *Ain, Zbits AblockType) {
  if ((((AblockType) & 2)))
  {
    CToken__FerrorNotAllowed(AfirstToken);
  }
  CNode *Vend_node;
  Vend_node = Anode_in;
  Vend_node = MParse__FnewNode(Vend_node, AfirstToken);
  Vend_node->Vn_attr = Aattr;
  CToken *Vtoken;
  Vtoken = CInput__FgetToken(Ain);
  if ((Vtoken->Vtype != 65))
  {
    CInput__FpushToken(Ain, Vtoken);
  }
else
  {
    Vtoken = CInput__FgetToken(Ain);
    if ((((AblockType) & 8)))
    {
      CToken__Ferror(Vtoken, "attribute not supported inside INTERFACE");
    }
  else if ((Vtoken->Vtype != 18))
    {
      CToken__Ferror(Vtoken, Zconcat("Unexpected item ", Zenum2string(CToken__EType, sizeof(CToken__EType) / sizeof(char *), Vtoken->Vtype)));
    }
  else if ((Zstrcmp(Vtoken->Vvalue, "default") == 0))
    {
      Vend_node->Vn_attr = ((Vend_node->Vn_attr) & -3) | ((1) << 1);
    }
  else if ((Zstrcmp(Vtoken->Vvalue, "abstract") == 0))
    {
      Vend_node->Vn_attr = ((Vend_node->Vn_attr) & -2) | ((1));
    }
  else
    {
      CToken__Ferror(Vtoken, Zconcat(Vtoken->Vvalue, " attribute not supported"));
    }
  }
  if ((((AblockType) & 8)))
  {
    Vend_node->Vn_attr = ((Vend_node->Vn_attr) & -2) | ((1));
  }
  if ((AfirstToken->Vtype == 39))
  {
    Vend_node->Vn_type = 33;
  }
else if ((AfirstToken->Vtype == 40))
  {
    Vend_node->Vn_type = 32;
  }
else if ((AfirstToken->Vtype == 31))
  {
    MParse__FexpectSep(Ain);
    CToken *Vnext;
    Vnext = CInput__FgetToken(Ain);
    if (((Vnext->Vtype == 18) && (Zstrcmp(Vnext->Vvalue, "proc") != 0)))
    {
      CToken *Vnnext;
      Vnnext = CInput__FgetToken(Ain);
      if ((Vnnext->Vtype == 66))
      {
        CToken__Ferror(Vnext, "Missing return type");
      }
      CInput__FpushToken(Ain, Vnnext);
    }
    CInput__FpushToken(Ain, Vnext);
    Vend_node->Vn_returnType = MParse__FparseDotName(Ain);
    Vend_node->Vn_type = 34;
  }
else
  {
    Vend_node->Vn_type = 35;
  }
  if (((AfirstToken->Vtype != 39) && (AfirstToken->Vtype != 40)))
  {
    MParse__FexpectSep(Ain);
    CToken *Vname;
    Vname = CInput__FgetToken(Ain);
    if ((Vname->Vtype != 18))
    {
      CToken__Ferror(Vname, "Expected a name");
    }
    Vend_node->Vn_string = Vname->Vvalue;
  }
  Vtoken = CInput__FgetToken(Ain);
  if ((Vtoken->Vtype != 66))
  {
    CToken__Ferror(Vtoken, "Missing (");
  }
else
  {
    CNode *Varg_node;
    Varg_node = Vend_node;
    MParse__FskipLineSep(Ain);
    while (1)
    {
      Vtoken = CInput__FgetToken(Ain);
      if ((Vtoken->Vtype == 67))
      {
        break;
      }
      if ((Varg_node == Vend_node))
      {
        CInput__FpushToken(Ain, Vtoken);
      }
    else
      {
        if ((Vtoken->Vtype != 61))
        {
          CToken__Ferror(Vtoken, "missing comma");
        }
        MParse__FexpectSep(Ain);
      }
      CNode *Varg_type;
      Varg_type = MParse__FparseDotName(Ain);
      MParse__FexpectSep(Ain);
      Vtoken = CInput__FgetToken(Ain);
      if ((Vtoken->Vtype == 83))
      {
        CNode *Vn;
        Vn = CNode__FNEW(6);
        Vn->Vn_left = Varg_type;
        Vn->Vn_start = Vtoken->VstartPos;
        Varg_type = Vn;
        Vtoken = CInput__FgetToken(Ain);
      }
      if ((Vtoken->Vtype != 18))
      {
        CToken__Ferror(Vtoken, "Expected argument name; missing )?");
        break;
      }
      CNode *Van;
      Van = CNode__FNEW(5);
      if ((Varg_node == Vend_node))
      {
        Vend_node->Vn_left = Van;
      }
    else
      {
        Varg_node->Vn_next = Van;
      }
      Van->Vn_string = Vtoken->Vvalue;
      Van->Vn_left = Varg_type;
      Van->Vn_start = Vtoken->VstartPos;
      Varg_node = Van;
      Vtoken = MParse__FtokenAfterSep(Ain);
      if ((Vtoken->Vtype != 61))
      {
        MParse__FskipLineSep(Ain);
      }
    }
  }
  MParse__FexpectNewLine(Ain);
  if (!((((Vend_node->Vn_attr) & 1))))
  {
    Vend_node->Vn_right = MParse__FparseBlock(Ain, 0);
  }
  return Vend_node;
}
CNode *MParse__FparseExpr(CInput *Ain) {
  return MParse__FparseExprAlt(Ain);
}
CNode *MParse__FparseRefExpr(CInput *Ain) {
  CToken *Vtoken;
  Vtoken = CInput__FgetToken(Ain);
  if ((Vtoken->Vtype == 83))
  {
    CNode *Vnode;
    Vnode = CNode__FNEW(6);
    Vnode->Vn_left = MParse__FparseExpr(Ain);
    Vnode->Vn_start = Vtoken->VstartPos;
    return Vnode;
  }
  CInput__FpushToken(Ain, Vtoken);
  return MParse__FparseExpr(Ain);
}
CNode *MParse__FparseExprAlt(CInput *Ain) {
  CNode *Vnode;
  Vnode = MParse__FparseExprOr(Ain);
  CToken *Vtoken;
  Vtoken = MParse__FtokenAfterSep(Ain);
  if ((Vtoken->Vtype == 64))
  {
    MParse__FexpectSep(Ain);
    Vtoken = CInput__FgetToken(Ain);
    MParse__FexpectSep(Ain);
    CNode *ValtNode;
    ValtNode = MParse__FnewOp(Vnode, Ain);
    ValtNode->Vn_cond = Vnode;
    Vnode = ValtNode;
    Vnode->Vn_type = 103;
    Vnode->Vn_left = MParse__FparseExprAlt(Ain);
    MParse__FexpectSep(Ain);
    Vtoken = CInput__FgetToken(Ain);
    if ((Vtoken->Vtype != 63))
    {
      CToken__Ferror(Vtoken, "Missing ':' after '?'");
      CInput__FpushToken(Ain, Vtoken);
    }
  else
    {
      MParse__FexpectSep(Ain);
      Vnode->Vn_right = MParse__FparseExprAlt(Ain);
    }
  }
  return Vnode;
}
CNode *MParse__FparseExprOr(CInput *Ain) {
  CNode *Vnode;
  Vnode = MParse__FparseExprAnd(Ain);
  while (1)
  {
    CToken *Vtoken;
    Vtoken = MParse__FtokenAfterSep(Ain);
    if ((Vtoken->Vtype != 102))
    {
      break;
    }
    MParse__FexpectSep(Ain);
    Vtoken = CInput__FgetToken(Ain);
    MParse__FexpectSep(Ain);
    Vnode = MParse__FnewOp(Vnode, Ain);
    Vnode->Vn_type = 102;
    Vnode->Vn_right = MParse__FparseExprAnd(Ain);
  }
  return Vnode;
}
CNode *MParse__FparseExprAnd(CInput *Ain) {
  CNode *Vnode;
  Vnode = MParse__FparseExprComp(Ain);
  while (1)
  {
    CToken *Vtoken;
    Vtoken = MParse__FtokenAfterSep(Ain);
    if ((Vtoken->Vtype != 101))
    {
      break;
    }
    MParse__FexpectSep(Ain);
    Vtoken = CInput__FgetToken(Ain);
    MParse__FexpectSep(Ain);
    Vnode = MParse__FnewOp(Vnode, Ain);
    Vnode->Vn_type = 101;
    Vnode->Vn_right = MParse__FparseExprComp(Ain);
  }
  return Vnode;
}
CNode *MParse__FparseExprComp(CInput *Ain) {
  CNode *Vnode;
  Vnode = MParse__FparseExprConcat(Ain);
  while (1)
  {
    CToken *Vtoken;
    Vtoken = MParse__FtokenAfterSep(Ain);
    if (((((((((((Vtoken->Vtype != 99) && (Vtoken->Vtype != 100)) && (Vtoken->Vtype != 104)) && (Vtoken->Vtype != 105)) && (Vtoken->Vtype != 106)) && (Vtoken->Vtype != 107)) && (Vtoken->Vtype != 108)) && (Vtoken->Vtype != 109)) && (Vtoken->Vtype != 110)) && (Vtoken->Vtype != 111)))
    {
      break;
    }
    Zbool VtypeSpec;
    VtypeSpec = 0;
    CToken *Vfirst;
    Vfirst = CInput__FgetToken(Ain);
    if ((Vfirst->Vtype == 106))
    {
      VtypeSpec = 1;
      Vtoken = Vfirst;
    }
  else
    {
      CInput__FpushToken(Ain, Vfirst);
      MParse__FexpectSep(Ain);
      Vtoken = CInput__FgetToken(Ain);
    }
    if (VtypeSpec)
    {
      MParse__FskipSep(Ain);
    }
  else
    {
      MParse__FexpectSep(Ain);
    }
    Vnode = MParse__FnewOp(Vnode, Ain);
    switch (Vtoken->Vtype)
    {
    case 99:
      {
        {
          Vnode->Vn_type = 91;
        }
          break;
      }
    case 100:
      {
        {
          Vnode->Vn_type = 92;
        }
          break;
      }
    case 104:
      {
        {
          Vnode->Vn_type = 93;
        }
          break;
      }
    case 105:
      {
        {
          Vnode->Vn_type = 94;
        }
          break;
      }
    case 106:
      {
        {
          Vnode->Vn_type = 95;
        }
          break;
      }
    case 107:
      {
        {
          Vnode->Vn_type = 96;
        }
          break;
      }
    case 108:
      {
        {
          Vnode->Vn_type = 97;
        }
          break;
      }
    case 109:
      {
        {
          Vnode->Vn_type = 98;
        }
          break;
      }
    case 110:
      {
        {
          Vnode->Vn_type = 99;
        }
          break;
      }
    case 111:
      {
        {
          Vnode->Vn_type = 100;
        }
          break;
      }
    }
    Vnode->Vn_right = MParse__FparseExprConcat(Ain);
    if ((Vnode->Vn_type == 95))
    {
      Vtoken = CInput__FgetToken(Ain);
      if (((Vtoken->Vtype == 104) || (Vtoken->Vtype == 61)))
      {
        Vnode->Vn_type = 109;
        if ((Vtoken->Vtype == 61))
        {
          Vnode->Vn_right->Vn_right = MParse__FparseNameList(Ain);
          CToken__Ferror(Vtoken, "comma in typespec not supported yet");
          Vtoken = CInput__FgetToken(Ain);
        }
        if ((Vtoken->Vtype != 104))
        {
          CToken__Ferror(Vtoken, "MISSING > after type spec");
          CInput__FpushToken(Ain, Vtoken);
        }
        Vnode = MParse__FparseMembers(Vnode, Ain, 0);
      }
    else
      {
        CInput__FpushToken(Ain, Vtoken);
      }
    }
  }
  return Vnode;
}
CNode *MParse__FparseExprConcat(CInput *Ain) {
  CNode *Vnode;
  Vnode = MParse__FparseExprBitwise(Ain);
  while (1)
  {
    CToken *Vtoken;
    Vtoken = MParse__FtokenAfterSep(Ain);
    if ((Vtoken->Vtype != 80))
    {
      break;
    }
    MParse__FexpectSep(Ain);
    Vtoken = CInput__FgetToken(Ain);
    MParse__FexpectSep(Ain);
    Vnode = MParse__FnewOp(Vnode, Ain);
    Vnode->Vn_type = 76;
    Vnode->Vn_right = MParse__FparseExprBitwise(Ain);
  }
  return Vnode;
}
CNode *MParse__FparseExprBitwise(CInput *Ain) {
  CNode *Vnode;
  Vnode = MParse__FparseExprShift(Ain);
  while (1)
  {
    CToken *Vtoken;
    Vtoken = MParse__FtokenAfterSep(Ain);
    if ((((Vtoken->Vtype != 83) && (Vtoken->Vtype != 81)) && (Vtoken->Vtype != 82)))
    {
      break;
    }
    MParse__FexpectSep(Ain);
    Vtoken = CInput__FgetToken(Ain);
    MParse__FexpectSep(Ain);
    Vnode = MParse__FnewOp(Vnode, Ain);
    if ((Vtoken->Vtype == 83))
    {
      Vnode->Vn_type = 73;
    }
  else if ((Vtoken->Vtype == 81))
    {
      Vnode->Vn_type = 74;
    }
  else
    {
      Vnode->Vn_type = 75;
    }
    Vnode->Vn_right = MParse__FparseExprShift(Ain);
  }
  return Vnode;
}
CNode *MParse__FparseExprShift(CInput *Ain) {
  CNode *Vnode;
  Vnode = MParse__FparseExprPlus(Ain);
  while (1)
  {
    CToken *Vtoken;
    Vtoken = MParse__FtokenAfterSep(Ain);
    if (((Vtoken->Vtype != 85) && (Vtoken->Vtype != 84)))
    {
      break;
    }
    MParse__FexpectSep(Ain);
    Vtoken = CInput__FgetToken(Ain);
    MParse__FexpectSep(Ain);
    Vnode = MParse__FnewOp(Vnode, Ain);
    if ((Vtoken->Vtype == 85))
    {
      Vnode->Vn_type = 90;
    }
  else
    {
      Vnode->Vn_type = 89;
    }
    Vnode->Vn_right = MParse__FparseExprPlus(Ain);
  }
  return Vnode;
}
CNode *MParse__FparseExprPlus(CInput *Ain) {
  CNode *Vnode;
  Vnode = MParse__FparseExprMult(Ain);
  while (1)
  {
    CToken *Vtoken;
    Vtoken = MParse__FtokenAfterSep(Ain);
    if (((Vtoken->Vtype != 74) && (Vtoken->Vtype != 72)))
    {
      break;
    }
    MParse__FexpectSep(Ain);
    Vtoken = CInput__FgetToken(Ain);
    MParse__FexpectSep(Ain);
    Vnode = MParse__FnewOp(Vnode, Ain);
    if ((Vtoken->Vtype == 74))
    {
      Vnode->Vn_type = 80;
    }
  else
    {
      Vnode->Vn_type = 81;
    }
    Vnode->Vn_right = MParse__FparseExprMult(Ain);
  }
  return Vnode;
}
CNode *MParse__FparseExprMult(CInput *Ain) {
  CNode *Vnode;
  Vnode = MParse__FparseExprIndecr(Ain);
  while (1)
  {
    CToken *Vtoken;
    Vtoken = MParse__FtokenAfterSep(Ain);
    if ((((Vtoken->Vtype != 77) && (Vtoken->Vtype != 78)) && (Vtoken->Vtype != 79)))
    {
      break;
    }
    MParse__FexpectSep(Ain);
    Vtoken = CInput__FgetToken(Ain);
    MParse__FexpectSep(Ain);
    Vnode = MParse__FnewOp(Vnode, Ain);
    if ((Vtoken->Vtype == 77))
    {
      Vnode->Vn_type = 77;
    }
  else if ((Vtoken->Vtype == 78))
    {
      Vnode->Vn_type = 78;
    }
  else
    {
      Vnode->Vn_type = 79;
    }
    Vnode->Vn_right = MParse__FparseExprIndecr(Ain);
  }
  return Vnode;
}
CNode *MParse__FparseExprIndecr(CInput *Ain) {
  CToken *Vtoken;
  Vtoken = CInput__FgetToken(Ain);
  CNode *Vpre_node;
  Vpre_node = NULL;
  if (((Vtoken->Vtype == 75) || (Vtoken->Vtype == 73)))
  {
    if ((Vtoken->Vtype == 75))
    {
      Vpre_node = CNode__FNEW(85);
    }
  else
    {
      Vpre_node = CNode__FNEW(86);
    }
  }
else
  {
    CInput__FpushToken(Ain, Vtoken);
  }
  CNode *Vnode;
  Vnode = MParse__FparseExprNeg(Ain);
  if ((Vpre_node != NULL))
  {
    Vpre_node->Vn_left = Vnode;
    Vnode = Vpre_node;
  }
  Vtoken = CInput__FgetToken(Ain);
  if ((Vtoken->Vtype == 2))
  {
    CToken *Vnext;
    Vnext = CInput__FgetToken(Ain);
    if ((Vnext->Vtype == 75))
    {
      CToken__Ferror(Vnext, "white space before ++");
      Vtoken = Vnext;
    }
  else if ((Vnext->Vtype == 73))
    {
      CToken__Ferror(Vnext, "white space before --");
      Vtoken = Vnext;
    }
  else
    {
      CInput__FpushToken(Ain, Vnext);
    }
  }
  if (((Vtoken->Vtype == 75) || (Vtoken->Vtype == 73)))
  {
    Vnode = MParse__FnewOp(Vnode, Ain);
    if ((Vtoken->Vtype == 75))
    {
      Vnode->Vn_type = 87;
    }
  else
    {
      Vnode->Vn_type = 88;
    }
  }
else
  {
    CInput__FpushToken(Ain, Vtoken);
  }
  return Vnode;
}
CNode *MParse__FparseExprNeg(CInput *Ain) {
  CNode *Vnode;
  Vnode = NULL;
  CToken *Vtoken;
  Vtoken = CInput__FgetToken(Ain);
  if ((Vtoken->Vtype == 72))
  {
    Vnode = CNode__FNEW(82);
    Vnode->Vn_left = MParse__FparseExprDot(Ain);
  }
else if ((Vtoken->Vtype == 103))
  {
    Vnode = CNode__FNEW(84);
    Vnode->Vn_left = MParse__FparseExprDot(Ain);
  }
else if ((Vtoken->Vtype == 76))
  {
    Vnode = CNode__FNEW(83);
    Vnode->Vn_left = MParse__FparseExprDot(Ain);
  }
else
  {
    CInput__FpushToken(Ain, Vtoken);
    Vnode = MParse__FparseExprDot(Ain);
  }
  return Vnode;
}
CNode *MParse__FparseExprDot(CInput *Ain) {
  CNode *Vnode;
  Vnode = MParse__FparseExprParen(Ain);
  return MParse__FparseMembers(Vnode, Ain, 0);
}
CNode *MParse__FparseExprParen(CInput *Ain) {
  CNode *Vnode;
  Vnode = NULL;
  CToken *Vtoken;
  Vtoken = CInput__FgetToken(Ain);
  if ((Vtoken->Vtype == 66))
  {
    Vnode = CNode__FNEW(104);
    MParse__FskipLineSep(Ain);
    Vnode->Vn_left = MParse__FparseExpr(Ain);
    MParse__FskipLineSep(Ain);
    Vtoken = CInput__FgetToken(Ain);
    if ((Vtoken->Vtype != 67))
    {
      CToken__Ferror(Vtoken, "missing )");
    }
  }
else
  {
    CInput__FpushToken(Ain, Vtoken);
    Vnode = MParse__FparseExprBase(Ain);
  }
  return Vnode;
}
CNode *MParse__FparseExprBase(CInput *Ain) {
  CNode *Vnode;
  Vnode = NULL;
  CToken *Vtoken;
  Vtoken = CInput__FgetToken(Ain);
  switch (Vtoken->Vtype)
  {
  case 13:
    {
      {
        Vnode = CNode__FNEW(9);
        Vnode->Vn_string = Vtoken->Vvalue;
      }
        break;
    }
  case 6:
    {
      {
        Vnode = CNode__FNEW(2);
      }
        break;
    }
  case 7:
    {
      {
        Vnode = CNode__FNEW(3);
      }
        break;
    }
  case 8:
    {
      {
        Vnode = CNode__FNEW(4);
      }
        break;
    }
  case 39:
    {
      {
        Vnode = CNode__FNEW(59);
        MParse__FparseNew(Vnode, Ain);
      }
        break;
    }
  case 10:
    {
      {
        Vnode = CNode__FNEW(11);
        Vnode->Vn_int = 1;
      }
        break;
    }
  case 9:
    {
      {
        Vnode = CNode__FNEW(11);
        Vnode->Vn_int = 0;
      }
        break;
    }
  case 11:
    {
      {
        Vnode = CNode__FNEW(12);
        Vnode->Vn_int = 1;
      }
        break;
    }
  case 12:
    {
      {
        Vnode = CNode__FNEW(12);
        Vnode->Vn_int = 0;
      }
        break;
    }
  case 62:
    {
      {
        CToken__Ferror(Vtoken, "unexpected ;");
      }
        break;
    }
  case 15:
    {
      {
        Vnode = CNode__FNEW(7);
        Vnode->Vn_int = Vtoken->Vvalue[0];
      }
        break;
    }
  case 18:
    {
      {
        Vnode = MParse__FparseId(Vtoken);
      }
        break;
    }
  case 70:
    {
      {
        Vnode = MParse__FparseList(Ain);
      }
        break;
    }
  case 68:
    {
      {
        Vnode = MParse__FparseDict(Ain);
      }
        break;
    }
  default:
    {
      {
        CInput__FpushToken(Ain, Vtoken);
        Vnode = CNode__FNEW(0);
      }
        break;
    }
  }
  Vnode->Vn_start = CPos__Fcopy(Vtoken->VstartPos);
  return Vnode;
}
CNode *MParse__FparseId(CToken *Atoken) {
  Zint Vc;
  Vc = Atoken->Vvalue[0];
  CNode *Vnode;
  Vnode = NULL;
  if (((Vc >= 48) && (Vc <= 57)))
  {
    Vnode = CNode__FNEW(7);
    if (((Vc == 48) && (((Atoken->Vvalue[1] == 120) || (Atoken->Vvalue[1] == 88)))))
    {
      Vnode->Vn_int = ZStringQuotedHexToInt(ZStringByteSlice(Atoken->Vvalue, 2, -(1)));
    }
  else if (((Vc == 48) && (((Atoken->Vvalue[1] == 98) || (Atoken->Vvalue[1] == 66)))))
    {
      Vnode->Vn_int = ZStringQuotedBinToInt(ZStringByteSlice(Atoken->Vvalue, 2, -(1)));
    }
  else
    {
      Vnode->Vn_int = ZStringQuotedToInt(Atoken->Vvalue);
    }
  }
else
  {
    Vnode = CNode__FNEW(5);
    Vnode->Vn_string = Atoken->Vvalue;
  }
  return Vnode;
}
void MParse__FparseNew(CNode *Anode, CInput *Ain) {
  Anode->Vn_type = 59;
  CToken *Vtoken;
  Vtoken = CInput__FgetToken(Ain);
  if ((Vtoken->Vtype != 66))
  {
    CToken__Ferror(Vtoken, Zconcat("Expected (, found ", Zenum2string(CToken__EType, sizeof(CToken__EType) / sizeof(char *), Vtoken->Vtype)));
    return ;
  }
  MParse__FskipLineSep(Ain);
  Anode->Vn_right = MParse__FparseComma(Ain, 2);
  Vtoken = CInput__FgetToken(Ain);
  if ((Vtoken->Vtype != 67))
  {
    CToken__Ferror(Vtoken, Zconcat("Expected ), found ", Zenum2string(CToken__EType, sizeof(CToken__EType) / sizeof(char *), Vtoken->Vtype)));
  }
}
CNode *MParse__FparseList(CInput *Ain) {
  CNode *Vnode;
  Vnode = CNode__FNEW(13);
  MParse__FskipLineSep(Ain);
  Vnode->Vn_right = MParse__FparseComma(Ain, 1);
  MParse__FskipLineSep(Ain);
  CToken *Vtoken;
  Vtoken = CInput__FgetToken(Ain);
  if ((Vtoken->Vtype != 71))
  {
    CToken__Ferror(Vtoken, Zconcat("Expected ], found ", Zenum2string(CToken__EType, sizeof(CToken__EType) / sizeof(char *), Vtoken->Vtype)));
  }
  return Vnode;
}
CNode *MParse__FparseDict(CInput *Ain) {
  CNode *Vnode;
  Vnode = CNode__FNEW(15);
  MParse__FskipLineSep(Ain);
  CToken *Vtoken;
  Vtoken = CInput__FgetToken(Ain);
  if ((Vtoken->Vtype == 69))
  {
    Vnode->Vn_type = 0;
  }
else
  {
    CInput__FpushToken(Ain, Vtoken);
    while (1)
    {
      Vnode->Vn_cond = MParse__FparseExpr(Ain);
      MParse__FskipLineSep(Ain);
      Vtoken = CInput__FgetToken(Ain);
      if ((Vtoken->Vtype != 63))
      {
        CToken__Ferror(Vtoken, "Missing ':' after dict key");
        if ((Vtoken->Vtype == 69))
        {
          CInput__FpushToken(Ain, Vtoken);
          break;
        }
      }
      MParse__FexpectSep(Ain);
      Vnode->Vn_right = MParse__FparseExpr(Ain);
      if ((MParse__FtokenAfterSep(Ain)->Vtype != 61))
      {
        if ((MParse__FtokenAfterSep(Ain)->Vtype == 69))
        {
          break;
        }
        CToken__Ferror(Vtoken, "missing comma");
      }
      Vtoken = CInput__FgetToken(Ain);
      if (CToken__FisSep(Vtoken))
      {
        CToken__Ferror(Vtoken, "superfluous white space");
        Vtoken = CInput__FgetToken(Ain);
      }
      if ((MParse__FtokenAfterSep(Ain)->Vtype == 69))
      {
        MParse__FskipSep(Ain);
        break;
      }
      MParse__FexpectSep(Ain);
      Vnode = MParse__FnewOp(Vnode, Ain);
      Vnode->Vn_type = 110;
    }
  }
  MParse__FskipLineSep(Ain);
  Vtoken = CInput__FgetToken(Ain);
  if ((Vtoken->Vtype != 69))
  {
    CToken__Ferror(Vtoken, Zconcat("Expected }, found ", Zenum2string(CToken__EType, sizeof(CToken__EType) / sizeof(char *), Vtoken->Vtype)));
  }
  CNode *VtopNode;
  VtopNode = CNode__FNEW(14);
  VtopNode->Vn_right = Vnode;
  return VtopNode;
}
CNode *MParse__FnewOp(CNode *Anode, CInput *Ain) {
  CNode *Vret_node;
  Vret_node = CNode__FNEW(0);
  Vret_node->Vn_left = Anode;
  Vret_node->Vn_start = CPos__Fcopy(Ain->Vpos);
  return Vret_node;
}
CNode *MParse__FparseDotName(CInput *Ain) {
  CToken *Vtoken;
  Vtoken = CInput__FgetToken(Ain);
  CNode *Vnode;
  Vnode = NULL;
  if ((Vtoken->Vtype == 18))
  {
    Vnode = CNode__FNEW(5);
  }
else if ((Vtoken->Vtype == 7))
  {
    Vnode = CNode__FNEW(3);
  }
else if ((Vtoken->Vtype == 8))
  {
    Vnode = CNode__FNEW(4);
  }
else
  {
    CToken__Ferror(Vtoken, Zconcat("unexpected token type: ", Zenum2string(CToken__EType, sizeof(CToken__EType) / sizeof(char *), Vtoken->Vtype)));
    Vnode = CNode__FNEW(0);
  }
  Vnode->Vn_string = Vtoken->Vvalue;
  Vnode->Vn_start = Vtoken->VstartPos;
  return MParse__FparseMembers(Vnode, Ain, 1);
}
CNode *MParse__FparseTypeName(CInput *Ain) {
  CToken *Vtoken;
  Vtoken = CInput__FgetToken(Ain);
  if (((Vtoken->Vtype != 17) && (Vtoken->Vtype != 16)))
  {
    CInput__FpushToken(Ain, Vtoken);
    return MParse__FparseDotName(Ain);
  }
  CNode *Vnode;
  Vnode = CNode__FNEW(((Vtoken->Vtype == 17)) ? (18) : (17));
  Vnode->Vn_string = Vtoken->Vvalue;
  Vnode->Vn_start = Vtoken->VstartPos;
  return Vnode;
}
CNode *MParse__FparseNameList(CInput *Ain) {
  CNode *Vnode;
  Vnode = MParse__FparseDotName(Ain);
  CToken *Vtoken;
  Vtoken = NULL;
  while (1)
  {
    Vtoken = CInput__FgetToken(Ain);
    if ((Vtoken->Vtype != 61))
    {
      break;
    }
    MParse__FexpectSep(Ain);
    Vnode = MParse__FnewOp(Vnode, Ain);
    Vnode->Vn_type = 110;
    Vnode->Vn_right = MParse__FparseDotName(Ain);
  }
  CInput__FpushToken(Ain, Vtoken);
  return Vnode;
}
CNode *MParse__FparseComma(CInput *Ain, Zbits Aflags) {
  CNode *Vnode;
  Vnode = NULL;
  if ((((Aflags) & 2)))
  {
    Vnode = MParse__FparseRefExpr(Ain);
  }
else
  {
    Vnode = MParse__FparseExpr(Ain);
  }
  while (1)
  {
    if ((MParse__FtokenAfterSep(Ain)->Vtype != 61))
    {
      break;
    }
    CToken *Vtoken;
    Vtoken = CInput__FgetToken(Ain);
    if ((Vtoken->Vtype != 61))
    {
      CToken__Ferror(Vtoken, "superfluous white space");
      Vtoken = CInput__FgetToken(Ain);
    }
    MParse__FexpectSep(Ain);
    CNode *Vright;
    Vright = NULL;
    if ((((Aflags) & 2)))
    {
      Vright = MParse__FparseRefExpr(Ain);
    }
  else
    {
      Vright = MParse__FparseExpr(Ain);
    }
    if ((Vright->Vn_type == 0))
    {
      if (!((((Aflags) & 1))))
      {
        CToken__Ferror(Vtoken, "trailing comma");
      }
      break;
    }
    Vnode = MParse__FnewOp(Vnode, Ain);
    Vnode->Vn_type = 110;
    Vnode->Vn_right = Vright;
  }
  return Vnode;
}
CNode *MParse__FparseMembers(CNode *Astart_node, CInput *Ain, Zbool AdoTypespec) {
  CNode *Vnode;
  Vnode = Astart_node;
  CToken *Vtoken;
  Vtoken = NULL;
  while (1)
  {
    Vtoken = MParse__FtokenAfterSep(Ain);
    if ((((((Vtoken->Vtype != 65) && (Vtoken->Vtype != 66)) && (Vtoken->Vtype != 70)) && (((Vtoken->Vtype != 86) || AdoTypespec))) && !(((AdoTypespec && (Vtoken->Vtype == 106))))))
    {
      break;
    }
    if ((Vtoken->Vtype == 65))
    {
      Vtoken = CInput__FgetToken(Ain);
      if ((Vtoken->Vtype != 3))
      {
        CInput__FpushToken(Ain, Vtoken);
      }
    }
    MParse__FcheckNoSep(Ain);
    Vtoken = CInput__FgetToken(Ain);
    if (((Vtoken->Vtype != 65) && (Vtoken->Vtype != 86)))
    {
      MParse__FskipLineSep(Ain);
    }
    Vnode = MParse__FnewOp(Vnode, Ain);
    if ((Vtoken->Vtype == 65))
    {
      Vtoken = CInput__FgetToken(Ain);
      if ((Vtoken->Vtype == 39))
      {
        MParse__FparseNew(Vnode, Ain);
      }
    else
      {
        if (((((Vtoken->Vtype != 18) && (Vtoken->Vtype != 41)) && (Vtoken->Vtype != 42)) && (Vtoken->Vtype != 43)))
        {
          CToken__Ferror(Vtoken, "expected ID after .");
        }
        Vnode->Vn_type = 105;
        Vnode->Vn_string = Vtoken->Vvalue;
      }
    }
  else if ((Vtoken->Vtype == 86))
    {
      Vtoken = CInput__FgetToken(Ain);
      Zbool Vnegative;
      Vnegative = 0;
      if ((Vtoken->Vtype == 72))
      {
        Vnegative = 1;
        Vtoken = CInput__FgetToken(Ain);
      }
      if ((Vtoken->Vtype != 18))
      {
        CToken__Ferror(Vtoken, "expected ID or number after =");
      }
      Vnode->Vn_type = 106;
      Vnode->Vn_right = MParse__FparseId(Vtoken);
      if (Vnegative)
      {
        if ((Vnode->Vn_right->Vn_type != 7))
        {
          CToken__Ferror(Vtoken, "expected number after =-");
        }
        Vnode->Vn_right->Vn_int = -(Vnode->Vn_right->Vn_int);
      }
    }
  else if ((Vtoken->Vtype == 66))
    {
      Vnode->Vn_type = 107;
      MParse__FskipLineSep(Ain);
      Vnode->Vn_right = MParse__FparseComma(Ain, 2);
      MParse__FskipLineSep(Ain);
      Vtoken = CInput__FgetToken(Ain);
      if ((Vtoken->Vtype != 67))
      {
        CToken__Ferror(Vtoken, "MISSING )");
        CInput__FpushToken(Ain, Vtoken);
      }
    }
  else if ((Vtoken->Vtype == 70))
    {
      Vnode->Vn_type = 108;
      Vnode->Vn_string = NULL;
      Vnode->Vn_right = MParse__FparseExpr(Ain);
      MParse__FskipLineSep(Ain);
      Vtoken = CInput__FgetToken(Ain);
      if ((Vtoken->Vtype != 71))
      {
        CToken__Ferror(Vtoken, "MISSING ] after subscript");
        CInput__FpushToken(Ain, Vtoken);
      }
    }
  else
    {
      Vnode->Vn_type = 109;
      Vnode->Vn_string = NULL;
      MParse__FskipLineSep(Ain);
      Vtoken = CInput__FgetToken(Ain);
      if ((((Vtoken->Vtype != 104) && (Vtoken->Vtype != 84)) && (Vtoken->Vtype != 21)))
      {
        CInput__FpushToken(Ain, Vtoken);
        Vnode->Vn_right = MParse__FparseNameList(Ain);
        MParse__FskipLineSep(Ain);
        Vtoken = CInput__FgetToken(Ain);
      }
      if (((Vtoken->Vtype == 84) || (Vtoken->Vtype == 21)))
      {
        CToken *Vsecond;
        Vsecond = CToken__Fcopy(Vtoken);
        Vsecond->Vtype = 104;
        ++(Vsecond->VstartPos->Vcol);
        if ((Vtoken->VendPos != NULL))
        {
          --(Vtoken->VendPos->Vcol);
        }
        if ((Vtoken->Vtype == 21))
        {
          CToken *Vthird;
          Vthird = CToken__Fcopy(Vsecond);
          ++(Vthird->VstartPos->Vcol);
          CInput__FpushToken(Ain, Vthird);
          if ((Vtoken->VendPos != NULL))
          {
            --(Vtoken->VendPos->Vcol);
            --(Vsecond->VendPos->Vcol);
          }
        }
        CInput__FpushToken(Ain, Vsecond);
        Vtoken->Vtype = 104;
      }
      if ((Vtoken->Vtype != 104))
      {
        CToken__Ferror(Vtoken, "MISSING > after type spec");
        CInput__FpushToken(Ain, Vtoken);
      }
    }
  }
  return Vnode;
}
CNode *MParse__FcopyCode(CInput *Ain) {
  Zint Vc;
  Vc = 0;
  CNode *Vnode;
  Vnode = CNode__FNEW(40);
  Zbool Vcomment;
  Vcomment = 0;
  while (1)
  {
    CPos *VstartPos;
    VstartPos = CPos__Fcopy(Ain->Vpos);
    Vc = CInput__Fget(Ain);
    if (((Vc == MIO__Veof) || (Vc == 10)))
    {
      break;
    }
    if ((Vc == 35))
    {
      Vcomment = 1;
    }
  else if ((!(Vcomment) && (Vc != 32)))
    {
      MError__Freport__1("Only comment allowed here", VstartPos);
      Vcomment = 1;
    }
  }
  Vnode->Vn_start = CPos__Fcopy(Ain->Vpos);
  Zint Vtext_len;
  Vtext_len = 200;
  char *Vtext;
  Vtext = malloc(Vtext_len);
  Zint Vidx;
  Vidx = 0;
  Zint Vcount;
  Vcount = 0;
  while (1)
  {
    Vc = CInput__Fget(Ain);
    if ((Vc == MIO__Veof))
    {
      MParse__Ferror("missing <<<", Ain);
      break;
    }
    if ((Vc == 10))
    {
      Vcount = 0;
    }
  else if (((Vcount >= 0) && (Vc == 60)))
    {
      if ((++(Vcount) == 3))
      {
        Vidx = (Vidx - 2);
        break;
      }
    }
  else
    {
      Vcount = -(1);
    }
    if (((Vidx + 2) >= Vtext_len))
    {
      Vtext_len = (Vtext_len + 200);
      Vtext = realloc(Vtext, Vtext_len);
    }
    Vtext[(Vidx)++] = Vc;
  }
  Vtext[(Vidx)++] = 0;
  while (1)
  {
    Vc = CInput__Fget(Ain);
    if (((Vc == MIO__Veof) || (Vc == 10)))
    {
      break;
    }
  }
  Vnode->Vn_string = Vtext;
  return Vnode;
}
void MParse__Ferror(char *Amsg, CInput *Ain) {
  MError__Freport__1(Amsg, Ain->Vpos);
}
void MParse__FexpectLineSep(CInput *Ain) {
  MParse__FexpectLineSep__1(Ain, 1);
}
void MParse__FexpectNewLine(CInput *Ain) {
  MParse__FexpectLineSep__1(Ain, 0);
}
void MParse__FexpectLineSep__1(CInput *Ain, Zbool AallowSemicolon) {
  CToken *Vtoken;
  Vtoken = CInput__FgetToken(Ain);
  CToken *Vnext;
  Vnext = NULL;
  if ((Vtoken->Vtype == 2))
  {
    Vnext = CInput__FgetToken(Ain);
    Vtoken = Vnext;
  }
  if (AallowSemicolon)
  {
    if ((Vtoken->Vtype == 62))
    {
      Vtoken = CInput__FgetToken(Ain);
      if ((Vtoken->Vtype == 3))
      {
        CToken__Ferror(Vtoken, "unexpected semicolon");
      }
    else
      {
        CInput__FpushToken(Ain, Vtoken);
        MParse__FexpectSep(Ain);
      }
      return ;
    }
    if ((Vnext != NULL))
    {
      CInput__FpushToken(Ain, Vnext);
    }
  }
  if ((Vtoken->Vtype != 3))
  {
    if ((Vtoken->Vtype == 4))
    {
      CToken__Ferror(Vtoken, "missing white space before comment");
    }
  else if ((Vtoken->Vtype == 62))
    {
      Vnext = CInput__FgetToken(Ain);
      if ((Vnext->Vtype == 3))
      {
        CToken__Ferror(Vtoken, "unexpected semicolon");
      }
    else
      {
        CToken__Ferror(Vtoken, "semicolon not allowed here, line break required");
        if ((Vnext->Vtype != 2))
        {
          CInput__FpushToken(Ain, Vnext);
        }
      }
    }
  else
    {
      CToken__Ferror(Vtoken, "missing line break");
      if ((Vtoken->Vtype != 2))
      {
        CInput__FpushToken(Ain, Vtoken);
      }
    }
  }
}
void MParse__FexpectSep(CInput *Ain) {
  CToken *Vtoken;
  Vtoken = CInput__FgetToken(Ain);
  if (((Vtoken->Vtype != 2) && (Vtoken->Vtype != 3)))
  {
    CToken__Ferror(Vtoken, "white space required");
    if ((Vtoken->Vtype != 4))
    {
      CInput__FpushToken(Ain, Vtoken);
    }
  }
}
void MParse__FskipSep(CInput *Ain) {
  CToken *Vtoken;
  Vtoken = CInput__FgetToken(Ain);
  if (!(CToken__FisSep(Vtoken)))
  {
    if ((Vtoken->Vtype == 4))
    {
      CToken__Ferror(Vtoken, "white space required");
    }
  else
    {
      CInput__FpushToken(Ain, Vtoken);
    }
  }
}
void MParse__FskipLineSep(CInput *Ain) {
  CToken *Vtoken;
  Vtoken = CInput__FgetToken(Ain);
  if ((Vtoken->Vtype != 3))
  {
    if ((Vtoken->Vtype == 2))
    {
      CToken__Ferror(Vtoken, "superfluous white space");
    }
  else if ((Vtoken->Vtype == 4))
    {
      CToken__Ferror(Vtoken, "white space required");
    }
  else
    {
      CInput__FpushToken(Ain, Vtoken);
    }
  }
}
CToken *MParse__FtokenAfterSep(CInput *Ain) {
  CToken *Vres;
  Vres = NULL;
  CToken *Vtoken;
  Vtoken = CInput__FgetToken(Ain);
  if (((Vtoken->Vtype == 2) || (Vtoken->Vtype == 3)))
  {
    Vres = CInput__FgetToken(Ain);
    CInput__FpushToken(Ain, Vres);
  }
else
  {
    Vres = Vtoken;
  }
  CInput__FpushToken(Ain, Vtoken);
  return Vres;
}
void MParse__FcheckNoSep(CInput *Ain) {
  CToken *Vnext;
  Vnext = CInput__FgetToken(Ain);
  if (((Vnext->Vtype == 2) || (Vnext->Vtype == 3)))
  {
    CToken__Ferror(Vnext, "superfluous white space");
  }
else
  {
    CInput__FpushToken(Ain, Vnext);
  }
}
void Iparse() {
 static int done = 0;
 if (!done) {
  done = 1;
  Iinput();
  Iscope();
  Itokenize();
  Iusedfile();
 }
}
