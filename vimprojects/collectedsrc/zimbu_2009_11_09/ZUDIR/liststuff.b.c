#define INC_liststuff_B 1
/*
 * FUNCTION BODIES
 */
#ifndef INC_error_B
#include "../ZUDIR/error.b.c"
#endif
#ifndef INC_generate_B
#include "../ZUDIR/generate.b.c"
#endif
#ifndef INC_node_B
#include "../ZUDIR/node.b.c"
#endif
#ifndef INC_output_B
#include "../ZUDIR/output.b.c"
#endif
#ifndef INC_resolve_B
#include "../ZUDIR/resolve.b.c"
#endif
#ifndef INC_scontext_B
#include "../ZUDIR/scontext.b.c"
#endif
#ifndef INC_scope_B
#include "../ZUDIR/scope.b.c"
#endif
#ifndef INC_symbol_B
#include "../ZUDIR/symbol.b.c"
#endif
CSymbol *MListStuff__FgenerateMethodCall(CSymbol *Asp, CNode *Am_node, CSContext *Actx, CSymbol *AdestSym) {
  CNode *Vnode;
  Vnode = Am_node->Vn_left;
  CNode *Varg_node;
  Varg_node = Am_node->Vn_right;
  CNode *Vvar_node;
  Vvar_node = Vnode->Vn_left;
  char *Vmethod;
  Vmethod = Vnode->Vn_string;
  CSymbol *Vret;
  Vret = CSymbol__FNEW(0);
  Am_node->Vn_undefined = 0;
  Zbool VdoJS;
  VdoJS = (((*(Zenum *)(Actx->Vgen->ptr + CResolve_I__VtargetLang_off[Actx->Vgen->type])) == 2));
  if ((Zstrcmp(Vmethod, "toString") == 0))
  {
    if ((MError__FcheckArgCount(Varg_node, 0, 0, Vmethod) == 0))
    {
      Am_node->Vn_undefined = 3;
    }
  else
    {
      CScope__FaddUsedItem(Actx->Vscope, "int2string");
      CScope__FaddUsedItem(Actx->Vscope, "garray");
      CScope__FaddUsedItem(Actx->Vscope, "listToString");
      COutput__Fwrite(Actx->Vout, "ZListToString(");
      MListStuff__FgenerateVarname(Vvar_node, Actx, Asp);
      Am_node->Vn_undefined = Vvar_node->Vn_undefined;
      COutput__Fwrite(Actx->Vout, ")");
      Vret->Vtype = 9;
    }
  }
else if (((Zstrcmp(Vmethod, "add") == 0) || (Zstrcmp(Vmethod, "insert") == 0)))
  {
    if ((MError__FcheckArgCount(Varg_node, 1, 2, Vmethod) == 0))
    {
      Am_node->Vn_undefined = 7;
    }
  else
    {
      if ((Zstrcmp(Vmethod, "add") == 0))
      {
        CScope__FaddUsedItem(Actx->Vscope, "listAdd");
        COutput__Fwrite(Actx->Vout, "ZListAdd(");
      }
    else
      {
        CScope__FaddUsedItem(Actx->Vscope, "listInsert");
        COutput__Fwrite(Actx->Vout, "ZListInsert(");
      }
      MListStuff__FgenerateVarname(Vvar_node, Actx, Asp);
      Am_node->Vn_undefined = Vvar_node->Vn_undefined;
      COutput__Fwrite(Actx->Vout, ", ");
      CNode *VitemNode;
      VitemNode = NULL;
      if ((Varg_node->Vn_type == 110))
      {
        MListStuff__FgenExpr(Varg_node->Vn_right, Actx, CSymbol__X__Vint);
        VitemNode = Varg_node->Vn_left;
        Am_node->Vn_undefined += Varg_node->Vn_right->Vn_undefined;
      }
    else
      {
        if ((Zstrcmp(Vmethod, "add") == 0))
        {
          COutput__Fwrite(Actx->Vout, "-1");
        }
      else
        {
          COutput__Fwrite(Actx->Vout, "0");
        }
        VitemNode = Varg_node;
      }
      COutput__Fwrite(Actx->Vout, ", ");
      if ((Asp->VreturnSymbol != NULL))
      {
        if (VdoJS)
        {
          MListStuff__FgenExpr(VitemNode, Actx, Asp->VreturnSymbol);
          Am_node->Vn_undefined += VitemNode->Vn_undefined;
        }
      else
        {
          CSContext *VtmpCtx;
          VtmpCtx = CSContext__FcopyNewOut(Actx);
          CSymbol *Vsym;
          Vsym = MListStuff__FgenExpr(VitemNode, VtmpCtx, Asp->VreturnSymbol);
          Am_node->Vn_undefined += VitemNode->Vn_undefined;
          if ((((Asp->VreturnSymbol->Vtype == 25) && (Vsym != NULL)) && (Asp->VreturnSymbol->Vclass != NULL)))
          {
            if (((Vsym != NULL) && (Vsym->Vtype == 25)))
            {
              COutput__Fwrite(Actx->Vout, "0, ");
              COutput__Fappend(Actx->Vout, VtmpCtx->Vout);
              COutput__Fwrite(Actx->Vout, ", 1");
            }
          else if ((Vsym->Vclass != NULL))
            {
              Zint Vidx;
              Vidx = CSymbol__FchildIndex(Asp->VreturnSymbol->Vclass, Vsym->Vclass);
              if (((Vidx < 0) && Actx->Vout->Vwriting))
              {
                CNode__Ferror(VitemNode, "Type mismatch");
              }
              CScope__FaddUsedItem(Actx->Vscope, "allocZoref");
              COutput__Fwrite(Actx->Vout, "0 , ZallocZoref(");
              COutput__Fappend(Actx->Vout, VtmpCtx->Vout);
              COutput__Fwrite(Actx->Vout, Zconcat(Zconcat(", ", Zint2string(Vidx)), "), 1"));
            }
          }
        else
          {
            if (CSymbol__FisPointerType(Asp->VreturnSymbol))
            {
              COutput__Fwrite(Actx->Vout, "0, ");
            }
            COutput__Fappend(Actx->Vout, VtmpCtx->Vout);
            if (CSymbol__FisPointerType(Asp->VreturnSymbol))
            {
              COutput__Fwrite(Actx->Vout, ", 1");
            }
          else
            {
              COutput__Fwrite(Actx->Vout, ", NULL, 0");
            }
          }
        }
      }
    else
      {
        Am_node->Vn_undefined += 5;
      }
      COutput__Fwrite(Actx->Vout, ")");
    }
    Vret->Vtype = 13;
    Vret->VreturnSymbol = Asp->VreturnSymbol;
  }
else if ((Zstrcmp(Vmethod, "extend") == 0))
  {
    if ((MError__FcheckArgCount(Varg_node, 1, 1, Vmethod) == 0))
    {
      Am_node->Vn_undefined = 3;
    }
  else
    {
      CScope__FaddUsedItem(Actx->Vscope, "listExtend");
      COutput__Fwrite(Actx->Vout, "ZListExtend(");
      MListStuff__FgenerateVarname(Vvar_node, Actx, Asp);
      COutput__Fwrite(Actx->Vout, ", ");
      MListStuff__FgenExpr(Varg_node, Actx, Asp);
      COutput__Fwrite(Actx->Vout, ")");
      Am_node->Vn_undefined = (Vvar_node->Vn_undefined + Varg_node->Vn_undefined);
    }
    Vret->Vtype = 13;
    Vret->VreturnSymbol = Asp->VreturnSymbol;
  }
else if ((Zstrcmp(Vmethod, "clear") == 0))
  {
    if ((MError__FcheckArgCount(Varg_node, 0, 0, Vmethod) == 0))
    {
      Am_node->Vn_undefined = 3;
    }
  else
    {
      CScope__FaddUsedItem(Actx->Vscope, "listClear");
      COutput__Fwrite(Actx->Vout, "ZListClear(");
      MListStuff__FgenerateVarname(Vvar_node, Actx, Asp);
      Am_node->Vn_undefined = Vvar_node->Vn_undefined;
      COutput__Fwrite(Actx->Vout, ")");
      Vret->Vtype = 13;
      Vret->VreturnSymbol = Asp->VreturnSymbol;
    }
  }
else if ((Zstrcmp(Vmethod, "pop") == 0))
  {
    if ((MError__FcheckArgCount(Varg_node, 0, 2, Vmethod) == 0))
    {
      Am_node->Vn_undefined = 3;
    }
  else
    {
      Zbool VpopItem;
      VpopItem = 0;
      Zbool VhasArg;
      VhasArg = 0;
      if (((Varg_node == NULL) || (Varg_node->Vn_type == 0)))
      {
        VpopItem = 1;
      }
    else if ((Varg_node->Vn_type != 110))
      {
        VpopItem = 1;
        VhasArg = 1;
      }
      if (VpopItem)
      {
        CScope__FaddUsedItem(Actx->Vscope, "listPopItem");
        if (VdoJS)
        {
          COutput__Fwrite(Actx->Vout, "ZListPopItem(");
        }
      else if (CSymbol__FisPointerType(Asp->VreturnSymbol))
        {
          CScope__FaddUsedItem(Actx->Vscope, "listPopPtrItem");
          COutput__Fwrite(Actx->Vout, "ZListPopPtrItem(");
        }
      else
        {
          CScope__FaddUsedItem(Actx->Vscope, "listPopIntItem");
          COutput__Fwrite(Actx->Vout, "ZListPopIntItem(");
        }
        MListStuff__FgenerateVarname(Vvar_node, Actx, Asp);
        Am_node->Vn_undefined = Vvar_node->Vn_undefined;
        COutput__Fwrite(Actx->Vout, ", ");
        if (VhasArg)
        {
          MListStuff__FgenExpr(Varg_node, Actx, CSymbol__X__Vint);
          Am_node->Vn_undefined += Varg_node->Vn_undefined;
        }
      else
        {
          COutput__Fwrite(Actx->Vout, "-1");
        }
        COutput__Fwrite(Actx->Vout, ")");
        Vret = Asp->VreturnSymbol;
      }
    else
      {
        CScope__FaddUsedItem(Actx->Vscope, "listPopList");
        COutput__Fwrite(Actx->Vout, "ZListPopList(");
        MListStuff__FgenerateVarname(Vvar_node, Actx, Asp);
        COutput__Fwrite(Actx->Vout, ", ");
        MListStuff__FgenExpr(Varg_node->Vn_left, Actx, CSymbol__X__Vint);
        COutput__Fwrite(Actx->Vout, ", ");
        MListStuff__FgenExpr(Varg_node->Vn_right, Actx, CSymbol__X__Vint);
        COutput__Fwrite(Actx->Vout, ")");
        Vret->Vtype = 13;
        Vret->VreturnSymbol = Asp->VreturnSymbol;
        Am_node->Vn_undefined = ((Vvar_node->Vn_undefined + Varg_node->Vn_left->Vn_undefined) + Varg_node->Vn_right->Vn_undefined);
      }
    }
  }
else if ((Zstrcmp(Vmethod, "slice") == 0))
  {
    if ((MError__FcheckArgCount(Varg_node, 2, 2, Vmethod) == 0))
    {
      Am_node->Vn_undefined = 3;
    }
  else
    {
      CScope__FaddUsedItem(Actx->Vscope, "listSlice");
      COutput__Fwrite(Actx->Vout, "ZListSlice(");
      MListStuff__FgenerateVarname(Vvar_node, Actx, Asp);
      COutput__Fwrite(Actx->Vout, ", ");
      MListStuff__FgenExpr(Varg_node->Vn_left, Actx, CSymbol__X__Vint);
      COutput__Fwrite(Actx->Vout, ", ");
      MListStuff__FgenExpr(Varg_node->Vn_right, Actx, CSymbol__X__Vint);
      COutput__Fwrite(Actx->Vout, ")");
      Vret->Vtype = 13;
      Vret->VreturnSymbol = Asp->VreturnSymbol;
      Am_node->Vn_undefined = ((Vvar_node->Vn_undefined + Varg_node->Vn_left->Vn_undefined) + Varg_node->Vn_right->Vn_undefined);
    }
  }
else if ((Zstrcmp(Vmethod, "COPY") == 0))
  {
    if ((MError__FcheckArgCount(Varg_node, 0, 0, Vmethod) == 0))
    {
      Am_node->Vn_undefined = 3;
    }
  else
    {
      CScope__FaddUsedItem(Actx->Vscope, "listCopy");
      if (VdoJS)
      {
        MListStuff__FgenerateVarname(Vvar_node, Actx, Asp);
        COutput__Fwrite(Actx->Vout, ".slice()");
        Am_node->Vn_undefined = Vvar_node->Vn_undefined;
      }
    else
      {
        COutput__Fwrite(Actx->Vout, "ZListCopy(");
        MListStuff__FgenerateVarname(Vvar_node, Actx, Asp);
        Am_node->Vn_undefined = Vvar_node->Vn_undefined;
        COutput__Fwrite(Actx->Vout, ")");
      }
      Vret->Vtype = 13;
      Vret->VreturnSymbol = Asp->VreturnSymbol;
    }
  }
else if ((Zstrcmp(Vmethod, "SIZE") == 0))
  {
    if ((MError__FcheckArgCount(Varg_node, 0, 0, Vmethod) == 0))
    {
      Am_node->Vn_undefined = 3;
    }
  else if (VdoJS)
    {
      MListStuff__FgenerateVarname(Vvar_node, Actx, Asp);
      Am_node->Vn_undefined = Vvar_node->Vn_undefined;
      COutput__Fwrite(Actx->Vout, ".length");
    }
  else
    {
      MListStuff__FgenerateVarname(Vvar_node, Actx, Asp);
      Am_node->Vn_undefined = Vvar_node->Vn_undefined;
      COutput__Fwrite(Actx->Vout, "->itemCount");
    }
    Vret->Vtype = 7;
  }
else
  {
    Am_node->Vn_undefined = 5;
    if (MGenerate__FdoError(Actx->Vout))
    {
      CNode__Ferror(Vnode, Zconcat(Zconcat("Method ", Vmethod), "() not supported for List"));
    }
  }
  return Vret;
}
CSymbol *MListStuff__FgenerateSubscript(CSymbol *Asym, CNode *Anode, Zbool Alvalue, CSContext *Actx, CSymbol *AdestSym) {
  CScope__FaddUsedItem(Actx->Vscope, "listGet");
  if (((*(Zenum *)(Actx->Vgen->ptr + CResolve_I__VtargetLang_off[Actx->Vgen->type])) == 2))
  {
    COutput__Fwrite(Actx->Vout, "ZListGet(");
    MListStuff__FgenExpr(Anode->Vn_left, Actx, NULL);
    COutput__Fwrite(Actx->Vout, ", ");
    MListStuff__FgenExpr(Anode->Vn_right, Actx, CSymbol__X__Vint);
    COutput__Fwrite(Actx->Vout, ")");
  }
else
  {
    char *Vclose;
    Vclose = ")";
    if (((Asym->VreturnSymbol != NULL) && CSymbol__FisPointerType(Asym->VreturnSymbol)))
    {
      CScope__FaddUsedItem(Actx->Vscope, "listGetPtr");
      if (((Asym->VreturnSymbol->Vtype == 21) || (Asym->VreturnSymbol->Vtype == 24)))
      {
        COutput__Fwrite(Actx->Vout, Zconcat(Zconcat("((", Asym->VreturnSymbol->VcName), " *)"));
        Vclose = "))";
      }
      COutput__Fwrite(Actx->Vout, "ZListGetPtr(");
    }
  else if (Alvalue)
    {
      CScope__FaddUsedItem(Actx->Vscope, "listGetIntP");
      COutput__Fwrite(Actx->Vout, "*ZListGetIntP(");
    }
  else
    {
      CScope__FaddUsedItem(Actx->Vscope, "listGetInt");
      COutput__Fwrite(Actx->Vout, "ZListGetInt(");
    }
    MListStuff__FgenExpr(Anode->Vn_left, Actx, NULL);
    COutput__Fwrite(Actx->Vout, ", ");
    MListStuff__FgenExpr(Anode->Vn_right, Actx, CSymbol__X__Vint);
    Anode->Vn_undefined = (Anode->Vn_left->Vn_undefined + Anode->Vn_right->Vn_undefined);
    COutput__Fwrite(Actx->Vout, Vclose);
  }
  if ((Asym->VreturnSymbol != NULL))
  {
    return Asym->VreturnSymbol;
  }
  ++(Anode->Vn_undefined);
  return AdestSym;
}
CSymbol *MListStuff__FgenerateVarname(CNode *Anode, CSContext *Actx, CSymbol *AdestSym) {
  return MGenerate__FgenerateVarname(Anode, Actx, AdestSym);
}
CSymbol *MListStuff__FgenExpr(CNode *Anode, CSContext *Actx, CSymbol *AdestSym) {
  return MGenerate__FgenExpr(Anode, Actx, AdestSym);
}
CSymbol *MListStuff__FgenerateListPart_C(CNode *Anode, CSContext *Actx, CSymbol *AdestSym) {
  CScope__FaddUsedItem(Actx->Vscope, "list");
  CSymbol *Vsym;
  Vsym = NULL;
  CNode *VargNode;
  VargNode = NULL;
  if ((Anode->Vn_type == 0))
  {
    Vsym = AdestSym;
    CResolve_I__MwriteAlloc__string__CSContext_ptr[Actx->Vgen->type](Actx->Vgen->ptr, "CListHead", Actx);
  }
else
  {
    CScope__FaddUsedItem(Actx->Vscope, "listAdd");
    COutput__Fwrite(Actx->Vout, "ZListAdd(");
    if ((Anode->Vn_type == 110))
    {
      Vsym = MListStuff__FgenerateListPart_C(Anode->Vn_left, Actx, AdestSym);
      Anode->Vn_undefined = Anode->Vn_left->Vn_undefined;
      VargNode = Anode->Vn_right;
    }
  else
    {
      CResolve_I__MwriteAlloc__string__CSContext_ptr[Actx->Vgen->type](Actx->Vgen->ptr, "CListHead", Actx);
      Vsym = MListStuff__FgenExpr(Anode, CSContext__FcopyNoOut(Actx), AdestSym);
      VargNode = Anode;
      Anode->Vn_undefined = 0;
    }
    COutput__Fwrite(Actx->Vout, ", -1, ");
    if (((Vsym != NULL) && CSymbol__FisPointerType(Vsym)))
    {
      COutput__Fwrite(Actx->Vout, "0, ");
    }
    MListStuff__FgenExpr(VargNode, Actx, Vsym);
    Anode->Vn_undefined += VargNode->Vn_undefined;
    if (((Vsym == NULL) || !(CSymbol__FisPointerType(Vsym))))
    {
      COutput__Fwrite(Actx->Vout, ", NULL, 0");
    }
  else
    {
      COutput__Fwrite(Actx->Vout, ", 1");
    }
    COutput__Fwrite(Actx->Vout, ")");
  }
  return Vsym;
}
void MListStuff__FwriteTypedefs_C(CScope *AtopScope, FILE *Afd) {
  if (ZDictHas(AtopScope->VusedItems, 0, "list"))
  {
    fputs("\ntypedef struct CListHead__S CListHead;\ntypedef struct CListItem__S CListItem;\n", Afd);
  }
}
void MListStuff__FwriteDecl_C(CScope *AtopScope, FILE *Afd) {
  if (ZDictHas(AtopScope->VusedItems, 0, "list"))
  {
    fputs("\nstruct CListHead__S {\n  CListItem *first;\n  CListItem *last;\n  int itemCount;\n};\nstruct CListItem__S {\n  CListItem *next;\n  CListItem *prev;\n  union {\n    int Ival;\n    void *Pval;\n  };\n  int type;\n};\n", Afd);
  }
}
void MListStuff__FwriteBody_C(CScope *AtopScope, FILE *Afd) {
  if (((((((ZDictHas(AtopScope->VusedItems, 0, "listAdd") || ZDictHas(AtopScope->VusedItems, 0, "listGet")) || ZDictHas(AtopScope->VusedItems, 0, "listInsert")) || ZDictHas(AtopScope->VusedItems, 0, "listPopIntItem")) || ZDictHas(AtopScope->VusedItems, 0, "listPopPtrItem")) || ZDictHas(AtopScope->VusedItems, 0, "listPopList")) || ZDictHas(AtopScope->VusedItems, 0, "listSlice")))
  {
    fputs("\nCListItem *ZListFind(CListHead *head, int idx) {\n  if (head == NULL) Zerror(\"Attempt to search in NIL list\");\n  CListItem *item;\n  int n;\n  if (idx < 0) {\n    item = head->last;\n    n = -idx;\n    while (--n > 0 && item != NULL)\n      item = item->prev;\n  } else {\n    item = head->first;\n    n = idx;\n    while (--n >= 0 && item != NULL)\n      item = item->next;\n  }\n  return item;\n}\n", Afd);
  }
  if (ZDictHas(AtopScope->VusedItems, 0, "listToString"))
  {
    fputs("\nvoid *ZListToString(CListHead *head) {\n  if (head == NULL) Zerror(\"Attempt to get string of NIL list\");\n  CListItem *item = head->first;\n  garray_T *ga = Zalloc(sizeof(garray_T));\n  ga_append(ga, \"[\");\n  while (item != NULL) {\n    if (item->type == 1) {\n      ga_append(ga, \"\\\"\");\n      ga_append(ga, item->Pval);\n      ga_append(ga, \"\\\"\");\n    } else\n      ga_append(ga, Zint2string(item->Ival));\n    item = item->next;\n    if (item != NULL ) ga_append(ga, \", \");\n  }\n  ga_append(ga, \"]\");\n  return ga->data;\n}\n", Afd);
  }
  if (ZDictHas(AtopScope->VusedItems, 0, "listExtend"))
  {
    fputs("\nCListHead *ZListExtend(CListHead *head, CListHead *head2) {\n  if (head == NULL) Zerror(\"Attempt to extend NIL list\");\n  if (head2 == NULL) return head;  /* TODO: throw exception? */\n  CListItem *item = head2->first;\n  while (item != NULL) {\n    CListItem *newitem = Zalloc(sizeof(CListItem));\n    if (item->type == 1) {\n      newitem->Pval = item->Pval;\n      newitem->type = 1;\n    } else\n      newitem->Ival = item->Ival;\n    if (head->last == NULL) {\n      head->first = newitem;\n      head->last = newitem;\n    } else {\n      newitem->prev = head->last;\n      newitem->prev->next = newitem;\n      head->last = newitem;\n    }\n    ++head->itemCount;\n    item = item->next;\n  }\n  return head;\n}\n", Afd);
  }
  if ((ZDictHas(AtopScope->VusedItems, 0, "listAdd") || ZDictHas(AtopScope->VusedItems, 0, "listInsert")))
  {
    fputs("\nCListHead *ZListAdd(CListHead *head, int after, int nr, void *ptr, int type) {\n  if (head == NULL) Zerror(\"Attempt to append to NIL list\");\n  CListItem *item = Zalloc(sizeof(CListItem));\n  if (type == 0)\n    item->Ival = nr;\n  else\n    item->Pval = ptr;\n  item->type = type;\n  if (head->last == NULL) {\n    head->first = item;\n    head->last = item;\n  } else {\n    CListItem *afterItem = NULL;\n    if (after != -1) {\n      afterItem = ZListFind(head, after);\n    }\n    if (afterItem == NULL || afterItem == head->last) {\n      item->prev = head->last;\n      item->prev->next = item;\n      head->last = item;\n    } else {\n      item->next = afterItem->next;\n      item->next->prev = item;\n      item->prev = afterItem;\n      afterItem->next = item;\n    }\n  }\n  ++head->itemCount;\n  return head;\n}\n", Afd);
  }
  if (ZDictHas(AtopScope->VusedItems, 0, "listInsert"))
  {
    fputs("\nCListHead *ZListInsert(CListHead *head, int before, int nr, void *ptr, int type) {\n  if (head == NULL) Zerror(\"Attempt to insert in NIL list\");\n  CListItem *item;\n  CListItem *iitem;\n  if (head->first == NULL)\n    return ZListAdd(head, -1, nr, ptr, type);\n  iitem = ZListFind(head, before);\n  if (iitem == NULL) {\n    if (before >= 0)\n      return ZListAdd(head, -1, nr, ptr, type);\n    iitem = head->first;\n  }\n  item = Zalloc(sizeof(CListItem));\n  if (type == 0)\n    item->Ival = nr;\n  else\n    item->Pval = ptr;\n  item->type = type;\n  item->prev = iitem->prev;\n  item->next = iitem;\n  if (item->prev == NULL)\n    head->first = item;\n  else\n    item->prev->next = item;\n  item->next->prev = item;\n  ++head->itemCount;\n  return head;\n}\n", Afd);
  }
  if (ZDictHas(AtopScope->VusedItems, 0, "listClear"))
  {
    fputs("\nCListHead *ZListClear(CListHead *head) {\n  if (head == NULL) Zerror(\"Attempt to clear NIL list\");\n  head->first = NULL;  /* TODO: free items */\n  head->last = NULL;\n  head->itemCount = 0;\n  return head;\n}\n", Afd);
  }
  if (ZDictHas(AtopScope->VusedItems, 0, "listCopy"))
  {
    fputs("\nCListHead *ZListCopy(CListHead *head) {\n  if (head == NULL) Zerror(\"Attempt to copy NIL list\");\n  CListHead *newhead = Zalloc(sizeof(CListHead));\n  CListItem *prev = NULL;\n  CListItem *item = head->first;\n  while (item != NULL) {\n    CListItem *newitem = Zalloc(sizeof(CListItem));\n    if (item->type == 1) {\n      newitem->Pval = item->Pval;\n      newitem->type = 1;\n    } else\n      newitem->Ival = item->Ival;\n    newitem->prev = prev;\n    if (prev == NULL)\n      newhead->first = newitem;\n    else\n      prev->next = newitem;\n    prev = newitem;\n    item = item->next;\n  }\n  if (prev != NULL)\n    prev->next = NULL;\n  newhead->last = prev;\n  newhead->itemCount = head->itemCount;\n  return newhead;\n}\n", Afd);
  }
  if (ZDictHas(AtopScope->VusedItems, 0, "listGetInt"))
  {
    fputs("\nint ZListGetInt(CListHead *head, int idx) {\n  CListItem *item = ZListFind(head, idx);\n  if (item != NULL)\n    return item->Ival;\n  return 0;\n}\n", Afd);
  }
  if (ZDictHas(AtopScope->VusedItems, 0, "listGetIntP"))
  {
    fputs("\nint *ZListGetIntP(CListHead *head, int idx) {\n  CListItem *item = ZListFind(head, idx);\n  if (item != NULL)\n    return &item->Ival;\n  return NULL;\n}\n", Afd);
  }
  if (ZDictHas(AtopScope->VusedItems, 0, "listGetPtr"))
  {
    fputs("\nvoid *ZListGetPtr(CListHead *head, int idx) {\n  CListItem *item = ZListFind(head, idx);\n  if (item != NULL)\n    return item->Pval;\n  return NULL;\n}\n", Afd);
  }
  if (ZDictHas(AtopScope->VusedItems, 0, "listPopIntItem"))
  {
    fputs("\nint ZListPopIntItem(CListHead *head, int idx) {\n  CListItem *item = ZListFind(head, idx);\n  if (item != NULL) {\n    int v = item->Ival;\n    if (item->prev == NULL)\n      head->first = item->next;\n    else\n      item->prev->next = item->next;\n    if (item->next == NULL)\n      head->last = item->prev;\n    else\n      item->next->prev = item->prev;\n    /* TODO: free *item */\n    head->itemCount--;\n    return v;\n  }\n  return 0;\n}\n", Afd);
  }
  if (ZDictHas(AtopScope->VusedItems, 0, "listPopPtrItem"))
  {
    fputs("\nvoid *ZListPopPtrItem(CListHead *head, int idx) {\n  CListItem *item = ZListFind(head, idx);\n  if (item != NULL) {\n    void *p = item->Pval;\n    if (item->prev == NULL)\n      head->first = item->next;\n    else\n      item->prev->next = item->next;\n    if (item->next == NULL)\n      head->last = item->prev;\n    else\n      item->next->prev = item->prev;\n    /* TODO: free *item */\n    head->itemCount--;\n    return p;\n  }\n  return NULL;\n}\n", Afd);
  }
  if (ZDictHas(AtopScope->VusedItems, 0, "listPopList"))
  {
    fputs("\nCListHead *ZListPopList(CListHead *head, int i1, int i2) {\n  if (head == NULL) Zerror(\"Attempt to pop from NIL list\");\n  CListHead *newhead = Zalloc(sizeof(CListHead));\n  CListItem *item, *nitem;\n  int n;\n  int ai1 = i1 >= 0 ? i1 : head->itemCount + i1;\n  int ai2 = i2 >= 0 ? i2 : head->itemCount + i2;\n  if (ai1 >= head->itemCount || ai1 > ai2)\n    return newhead;\n  if (ai2 >= head->itemCount)\n    ai2 = head->itemCount - 1;\n  item = ZListFind(head, ai1);\n  for (n = ai1; n <= ai2; ++n) {\n    nitem = item->next;\n    if (item->prev == NULL)\n      head->first = item->next;\n    else\n      item->prev->next = item->next;\n    if (item->next == NULL)\n      head->last = item->prev;\n    else\n      item->next->prev = item->prev;\n    head->itemCount--;\n    if (newhead->last == NULL) {\n      newhead->first = item;\n      newhead->last = item;\n    } else {\n      item->prev = newhead->last;\n      item->prev->next = item;\n      newhead->last = item;\n    }\n    ++newhead->itemCount;\n    item = nitem;\n  }\n  return newhead;\n}\n", Afd);
  }
  if (ZDictHas(AtopScope->VusedItems, 0, "listSlice"))
  {
    fputs("\nCListHead *ZListSlice(CListHead *head, int i1, int i2) {\n  if (head == NULL) Zerror(\"Attempt to slice NIL list\");\n  CListHead *newhead = Zalloc(sizeof(CListHead));\n  CListItem *item;\n  int n;\n  int ai1 = i1 >= 0 ? i1 : head->itemCount + i1;\n  int ai2 = i2 >= 0 ? i2 : head->itemCount + i2;\n  if (ai1 < 0) ai1 = 0;\n  if (ai1 >= head->itemCount || ai1 > ai2)\n    return newhead;\n  if (ai2 >= head->itemCount)\n    ai2 = head->itemCount - 1;\n  item = ZListFind(head, ai1);\n  CListItem *prev = NULL;\n  for (n = ai1; n <= ai2; ++n) {\n    CListItem *newitem = Zalloc(sizeof(CListItem));\n    if (item->type == 1) {\n      newitem->Pval = item->Pval;\n      newitem->type = 1;\n    } else\n      newitem->Ival = item->Ival;\n    newitem->prev = prev;\n    if (prev == NULL)\n      newhead->first = newitem;\n    else\n      prev->next = newitem;\n    prev = newitem;\n    item = item->next;\n    ++newhead->itemCount;\n  }\n  return newhead;\n}\n", Afd);
  }
}
CSymbol *MListStuff__FgenerateListPart_JS(CNode *Anode, CSContext *Actx, CSymbol *AdestSym) {
  CScope__FaddUsedItem(Actx->Vscope, "list");
  COutput__Fwrite(Actx->Vout, "[");
  CSymbol *Vsym;
  Vsym = NULL;
  if ((Anode->Vn_type == 0))
  {
    Vsym = AdestSym;
  }
else
  {
    Vsym = MListStuff__FoneListPart_JS(Anode, Actx, AdestSym);
  }
  COutput__Fwrite(Actx->Vout, "]");
  return Vsym;
}
CSymbol *MListStuff__FoneListPart_JS(CNode *Anode, CSContext *Actx, CSymbol *AdestSym) {
  CSymbol *Vsym;
  Vsym = NULL;
  if ((Anode->Vn_type == 110))
  {
    Vsym = MListStuff__FoneListPart_JS(Anode->Vn_left, Actx, AdestSym);
    COutput__Fwrite(Actx->Vout, ", ");
    MListStuff__FgenExpr(Anode->Vn_right, Actx, Vsym);
    Anode->Vn_undefined = (Anode->Vn_left->Vn_undefined + Anode->Vn_right->Vn_undefined);
  }
else
  {
    Vsym = MListStuff__FgenExpr(Anode, Actx, AdestSym);
  }
  return Vsym;
}
void MListStuff__FwriteBody_JS(CScope *AtopScope, FILE *Afd) {
  if (ZDictHas(AtopScope->VusedItems, 0, "listAdd"))
  {
    fputs("\nfunction ZListAdd(head, after, val) {\n  if (head == null) Zerror(\"Attempt to append to NIL list\");\n  if (after == -1 || after >= head.length) {\n    head.push(val);\n  } else {\n    if (after < 0) {\n      var before = head.length + after + 1;\n      if (before < 0) {\n        before = 0;\n      }\n      head.splice(before, 0, val);\n    } else {\n      head.splice(after + 1, 0, val);\n    }\n  }\n  return head;\n}\n", Afd);
  }
  if (ZDictHas(AtopScope->VusedItems, 0, "listInsert"))
  {
    fputs("\nfunction ZListInsert(head, before, val) {\n  if (head == null) Zerror(\"Attempt to insert in NIL list\");\n  if (before >= 0) {\n    if (before > head.length) {\n      head.push(val);\n    } else {\n      head.splice(before, 0, val);\n    }\n  } else {\n    var idx = head.length + before;\n    if (idx < 0) {\n      idx = 0;\n    }\n    head.splice(idx, 0, val);\n  }\n  return head;\n}\n", Afd);
  }
  if (ZDictHas(AtopScope->VusedItems, 0, "listGet"))
  {
    fputs("\nfunction ZListGet(head, idx) {\n  if (idx >= 0) {\n    if (idx >= head.length) return null;\n    return head[idx];\n  }\n  var i = head.length + idx;\n  if (i < 0) return null;\n  return head[i];\n}\n", Afd);
  }
  if (ZDictHas(AtopScope->VusedItems, 0, "listToString"))
  {
    fputs("\nfunction ZListToString(head) {\n  if (head == null) Zerror(\"Attempt to get string of NIL list\");\n  var result = \"[\";\n  var comma = \"\";\n  var i;\n  for (i = 0; i < head.length; ++i) {\n    var item = head[i];\n    if (typeof item == \"string\") {\n      result = result + comma + '\"' + item + '\"';\n    } else {\n      result = result + comma + item;\n    }\n    comma = \", \";\n  }\n  return result + \"]\";\n}\n", Afd);
  }
  if (ZDictHas(AtopScope->VusedItems, 0, "listExtend"))
  {
    fputs("\nfunction ZListExtend(head, head2) {\n  if (head == null) Zerror(\"Attempt to extend NIL list\");\n  if (head2 == null) return head;\n  for (var i in head2) {\n    head.push(head2[i]);\n  }\n  return head;\n}\n", Afd);
  }
  if (ZDictHas(AtopScope->VusedItems, 0, "listClear"))
  {
    fputs("\nfunction ZListClear(head) {\n  if (head == null) Zerror(\"Attempt to clear NIL list\");\n  head.length = 0;\n  return head;\n}\n", Afd);
  }
  if (ZDictHas(AtopScope->VusedItems, 0, "listPopItem"))
  {
    fputs("\nfunction ZListPopItem(head, idx) {\n  if (head == null) Zerror(\"Attempt to pop from a NIL list\");\n  if (head.length == 0) return null;\n  if (idx == -1) return head.pop();\n  var i = idx;\n  if (idx < 0) {\n    i = head.length + idx;\n  }\n  if (i >= 0 && i < head.length) {\n    var a = head.splice(i, 1);\n    return a[0];\n  }\n  return null;\n}\n", Afd);
  }
  if (ZDictHas(AtopScope->VusedItems, 0, "listPopList"))
  {
    fputs("\nfunction ZListPopList(head, i1, i2) {\n  if (head == null) Zerror(\"Attempt to pop from NIL list\");\n  var idx1 = i1;\n  var idx2 = i2;\n  if (i1 < 0) {\n    idx1 = head.length + i1;\n  }\n  if (idx1 < 0) idx1 = 0;\n  if (idx1 >= head.length) idx1 = head.length - 1;\n  if (i2 < 0) {\n    idx2 = head.length + i2;\n  }\n  if (idx2 < 0) idx2 = 0;\n  if (idx2 >= head.length) idx2 = head.length - 1;\n  if (idx1 > idx2) return [];\n  return head.splice(idx1, idx2 - idx1 + 1);\n}\n", Afd);
  }
  if (ZDictHas(AtopScope->VusedItems, 0, "listSlice"))
  {
    fputs("\nfunction ZListSlice(head, i1, i2) {\n  if (head == null) Zerror(\"Attempt to slice NIL list\");\n  var idx1 = i1;\n  var idx2 = i2;\n  if (i1 < 0) {\n    idx1 = head.length + i1;\n  }\n  if (idx1 < 0) idx1 = 0;\n  if (idx1 >= head.length) idx1 = head.length - 1;\n  if (i2 < 0) {\n    idx2 = head.length + i2;\n  }\n  if (idx2 < 0) idx2 = 0;\n  if (idx2 >= head.length) idx2 = head.length - 1;\n  if (idx1 > idx2) return [];\n  return head.slice(idx1, idx2 + 1);\n}\n", Afd);
  }
}
void Iliststuff() {
 static int done = 0;
 if (!done) {
  done = 1;
  Igenerate();
  Ioutput();
  Iresolve();
  Iscontext();
  Iscope();
  Isymbol();
 }
}
