#define INC_dictstuff_B 1
/*
 * FUNCTION BODIES
 */
#ifndef INC_error_B
#include "../ZUDIR/error.b.c"
#endif
#ifndef INC_generate_B
#include "../ZUDIR/generate.b.c"
#endif
#ifndef INC_liststuff_B
#include "../ZUDIR/liststuff.b.c"
#endif
#ifndef INC_node_B
#include "../ZUDIR/node.b.c"
#endif
#ifndef INC_output_B
#include "../ZUDIR/output.b.c"
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
CSymbol *MDictStuff__FgenerateMethodCall(CSymbol *Asp, CNode *Am_node, CSContext *Actx, CSymbol *AdestSym) {
  CNode *Vnode;
  Vnode = Am_node->Vn_left;
  CNode *Vvar_node;
  Vvar_node = Vnode->Vn_left;
  CNode *Varg_node;
  Varg_node = Am_node->Vn_right;
  char *Vmethod;
  Vmethod = Vnode->Vn_string;
  CSymbol *Vret;
  Vret = CSymbol__FNEW(0);
  if ((Zstrcmp(Vmethod, "toString") == 0))
  {
    if ((MError__FcheckArgCount(Varg_node, 0, 0, Vmethod) == 1))
    {
      CScope__FaddUsedItem(Actx->Vscope, "int2string");
      CScope__FaddUsedItem(Actx->Vscope, "garray");
      MDictStuff__VuseZDictToString = 1;
      COutput__Fwrite(Actx->Vout, "ZDictToString(");
      MDictStuff__FgenerateVarname(Vvar_node, Actx, Asp);
      COutput__Fwrite(Actx->Vout, ")");
      Vret->Vtype = 9;
    }
  }
else if ((Zstrcmp(Vmethod, "get") == 0))
  {
    if ((MError__FcheckArgCount(Varg_node, 1, 2, Vmethod) == 1))
    {
      if ((Varg_node->Vn_type == 110))
      {
        Vret = MDictStuff__FgenerateGet(Asp, Vvar_node, Varg_node->Vn_left, Varg_node->Vn_right, Actx, AdestSym);
      }
    else
      {
        Vret = MDictStuff__FgenerateGet(Asp, Vvar_node, Varg_node, NULL, Actx, AdestSym);
      }
    }
  }
else if ((Zstrcmp(Vmethod, "has") == 0))
  {
    if ((MError__FcheckArgCount(Varg_node, 1, 1, Vmethod) == 1))
    {
      MDictStuff__VuseZDictHas = 1;
      COutput__Fwrite(Actx->Vout, "ZDictHas(");
      MDictStuff__FgenerateVarname(Vvar_node, Actx, Asp);
      MDictStuff__FgenKeyArgs(Asp->VkeySymbol, Varg_node, Actx);
      COutput__Fwrite(Actx->Vout, ")");
      Vret->Vtype = 11;
    }
  }
else if ((Zstrcmp(Vmethod, "add") == 0))
  {
    if ((MError__FcheckArgCount(Varg_node, 2, 2, Vmethod) == 1))
    {
      COutput__Fwrite(Actx->Vout, "ZDictAdd(0, ");
      MDictStuff__FgenerateVarname(Vvar_node, Actx, Asp);
      MDictStuff__FgenKeyArgs(Asp->VkeySymbol, Varg_node->Vn_left, Actx);
      MDictStuff__FgenKeyArgs(Asp->VreturnSymbol, Varg_node->Vn_right, Actx);
      COutput__Fwrite(Actx->Vout, ")");
      Vret->Vtype = 14;
      Vret->VkeySymbol = Asp->VkeySymbol;
      Vret->VreturnSymbol = Asp->VreturnSymbol;
    }
  }
else if ((Zstrcmp(Vmethod, "remove") == 0))
  {
    if ((MError__FcheckArgCount(Varg_node, 1, 1, Vmethod) == 1))
    {
      MDictStuff__VuseZDictRemove = 1;
      COutput__Fwrite(Actx->Vout, "ZDictRemove(");
      MDictStuff__FgenerateVarname(Vvar_node, Actx, Asp);
      MDictStuff__FgenKeyArgs(Asp->VkeySymbol, Varg_node, Actx);
      COutput__Fwrite(Actx->Vout, ")");
      Vret->Vtype = 14;
      Vret->VkeySymbol = Asp->VkeySymbol;
      Vret->VreturnSymbol = Asp->VreturnSymbol;
    }
  }
else if ((Zstrcmp(Vmethod, "clear") == 0))
  {
    if ((MError__FcheckArgCount(Varg_node, 0, 0, Vmethod) == 1))
    {
      MDictStuff__VuseZDictClear = 1;
      COutput__Fwrite(Actx->Vout, "ZDictClear(");
      MDictStuff__FgenerateVarname(Vvar_node, Actx, Asp);
      COutput__Fwrite(Actx->Vout, ")");
      Vret->Vtype = 14;
      Vret->VkeySymbol = Asp->VkeySymbol;
      Vret->VreturnSymbol = Asp->VreturnSymbol;
    }
  }
else if ((Zstrcmp(Vmethod, "keys") == 0))
  {
    if ((MError__FcheckArgCount(Varg_node, 0, 0, Vmethod) == 1))
    {
      MDictStuff__VuseZDictKeys = 1;
      CScope__FaddUsedItem(Actx->Vscope, "list");
      CScope__FaddUsedItem(Actx->Vscope, "listAdd");
      COutput__Fwrite(Actx->Vout, "ZDictKeys(");
      MDictStuff__FgenerateVarname(Vvar_node, Actx, Asp);
      COutput__Fwrite(Actx->Vout, ")");
      Vret->Vtype = 13;
      Vret->VreturnSymbol = Asp->VkeySymbol;
    }
  }
else if ((Zstrcmp(Vmethod, "COPY") == 0))
  {
    if ((MError__FcheckArgCount(Varg_node, 0, 0, Vmethod) == 1))
    {
      MDictStuff__VuseZDictCopy = 1;
      COutput__Fwrite(Actx->Vout, "ZDictCopy(");
      MDictStuff__FgenerateVarname(Vvar_node, Actx, Asp);
      COutput__Fwrite(Actx->Vout, ")");
      Vret->Vtype = 14;
      Vret->VkeySymbol = Asp->VkeySymbol;
      Vret->VreturnSymbol = Asp->VreturnSymbol;
    }
  }
else if ((Zstrcmp(Vmethod, "SIZE") == 0))
  {
    if ((MError__FcheckArgCount(Varg_node, 0, 0, Vmethod) == 1))
    {
      MDictStuff__FgenerateVarname(Vvar_node, Actx, Asp);
      COutput__Fwrite(Actx->Vout, "->used");
    }
    Vret->Vtype = 7;
  }
else if (MGenerate__FdoError(Actx->Vout))
  {
    CNode__Ferror(Vnode, Zconcat(Zconcat("Method ", Vmethod), "() not supported for Dict"));
  }
  return Vret;
}
CSymbol *MDictStuff__FgenerateVarname(CNode *Anode, CSContext *Actx, CSymbol *AdestSym) {
  return MGenerate__FgenerateVarname(Anode, Actx, AdestSym);
}
CSymbol *MDictStuff__FgenExpr(CNode *Anode, CSContext *Actx, CSymbol *AdestSym) {
  return MGenerate__FgenExpr(Anode, Actx, AdestSym);
}
CSymbol *MDictStuff__FgenerateSubscript(CSymbol *Asym, CNode *Anode, Zbool Alvalue, CSContext *Actx, CSymbol *AdestSym) {
  CSymbol *Vret;
  Vret = MDictStuff__FgenerateGet(Asym, Anode->Vn_left, Anode->Vn_right, NULL, Actx, AdestSym);
  Anode->Vn_undefined = (Anode->Vn_left->Vn_undefined + Anode->Vn_right->Vn_undefined);
  return Vret;
}
CSymbol *MDictStuff__FgenerateGet(CSymbol *Asym, CNode *Adict_node, CNode *Aarg_node, CNode *Adef_node, CSContext *Actx, CSymbol *AdestSym) {
  char *Vclose;
  Vclose = ")";
  if (((Asym->VreturnSymbol != NULL) && CSymbol__FisPointerType(Asym->VreturnSymbol)))
  {
    if ((Asym->VreturnSymbol->Vtype == 21))
    {
      COutput__Fwrite(Actx->Vout, Zconcat(Zconcat("((", Asym->VreturnSymbol->VcName), " *)"));
      Vclose = "))";
    }
  else if ((Asym->VreturnSymbol->Vtype == 25))
    {
      COutput__Fwrite(Actx->Vout, "((Zoref *)");
      Vclose = "))";
    }
    if ((Adef_node == NULL))
    {
      MDictStuff__VuseZDictGetPtr = 1;
      COutput__Fwrite(Actx->Vout, "ZDictGetPtr(");
    }
  else
    {
      MDictStuff__VuseZDictGetPtrDef = 1;
      COutput__Fwrite(Actx->Vout, "ZDictGetPtrDef(");
    }
  }
else
  {
    if ((Adef_node == NULL))
    {
      MDictStuff__VuseZDictGetInt = 1;
      COutput__Fwrite(Actx->Vout, "ZDictGetInt(");
    }
  else
    {
      MDictStuff__VuseZDictGetIntDef = 1;
      COutput__Fwrite(Actx->Vout, "ZDictGetIntDef(");
    }
  }
  MDictStuff__VuseZDictFind = 1;
  MDictStuff__FgenExpr(Adict_node, Actx, NULL);
  MDictStuff__FgenKeyArgs(Asym->VkeySymbol, Aarg_node, Actx);
  if ((Adef_node != NULL))
  {
    COutput__Fwrite(Actx->Vout, ", ");
    MDictStuff__FgenExpr(Adef_node, Actx, Asym->VreturnSymbol);
  }
  COutput__Fwrite(Actx->Vout, Vclose);
  if ((Asym->VreturnSymbol != NULL))
  {
    return Asym->VreturnSymbol;
  }
  return AdestSym;
}
void MDictStuff__FgenKeyArgs(CSymbol *Asym, CNode *Anode, CSContext *Actx) {
  COutput__Fwrite(Actx->Vout, ", ");
  Zbool VisPointer;
  VisPointer = 0;
  if ((Asym != NULL))
  {
    VisPointer = CSymbol__FisPointerType(Asym);
  }
  if (VisPointer)
  {
    COutput__Fwrite(Actx->Vout, "0, ");
  }
  MDictStuff__FgenExpr(Anode, Actx, ((Asym != NULL)) ? (Asym) : (CSymbol__X__Vint));
  if (!(VisPointer))
  {
    COutput__Fwrite(Actx->Vout, ", NULL");
  }
}
void MDictStuff__FgenerateDictPart_C(CNode *Anode, CSymbol *Aret, CSContext *Actx) {
  MDictStuff__VuseDict = 1;
  Zbool VkeyIsPointer;
  VkeyIsPointer = 0;
  COutput__Fwrite(Actx->Vout, "ZDictAdd(0, ");
  if ((Anode->Vn_type == 110))
  {
    MDictStuff__FgenerateDictPart_C(Anode->Vn_left, Aret, Actx);
    if ((Aret->VkeySymbol != NULL))
    {
      VkeyIsPointer = CSymbol__FisPointerType(Aret->VkeySymbol);
    }
  }
else
  {
    COutput *VnoOut;
    VnoOut = COutput__FNEW(NULL);
    Aret->VkeySymbol = MDictStuff__FgenExpr(Anode->Vn_cond, CSContext__FcopyNoOut(Actx), NULL);
    Aret->VreturnSymbol = MDictStuff__FgenExpr(Anode->Vn_right, CSContext__FcopyNoOut(Actx), NULL);
    if ((Aret->VkeySymbol != NULL))
    {
      VkeyIsPointer = CSymbol__FisPointerType(Aret->VkeySymbol);
    }
    if (VkeyIsPointer)
    {
      COutput__Fwrite(Actx->Vout, "ZnewDict(1)");
    }
  else
    {
      COutput__Fwrite(Actx->Vout, "ZnewDict(0)");
    }
  }
  COutput__Fwrite(Actx->Vout, ", ");
  if (VkeyIsPointer)
  {
    COutput__Fwrite(Actx->Vout, "0, ");
  }
  MDictStuff__FgenExpr(Anode->Vn_cond, Actx, Aret->VkeySymbol);
  if (!(CSymbol__FisPointerType(Aret->VkeySymbol)))
  {
    COutput__Fwrite(Actx->Vout, ", NULL");
  }
  COutput__Fwrite(Actx->Vout, ", ");
  if (CSymbol__FisPointerType(Aret->VreturnSymbol))
  {
    COutput__Fwrite(Actx->Vout, "0, ");
  }
  MDictStuff__FgenExpr(Anode->Vn_right, Actx, Aret->VreturnSymbol);
  if (!(CSymbol__FisPointerType(Aret->VreturnSymbol)))
  {
    COutput__Fwrite(Actx->Vout, ", NULL");
  }
  COutput__Fwrite(Actx->Vout, ")");
}
void MDictStuff__FwriteTypedefs(CScope *AtopScope, FILE *Afd) {
  if (MDictStuff__VuseDict)
  {
    fputs("\ntypedef unsigned long Zhashtype;\ntypedef struct CDictItem__S CDictItem;\ntypedef struct CDictHead__S CDictHead;\n", Afd);
  }
}
void MDictStuff__FwriteDecl(CScope *AtopScope, FILE *Afd) {
  if (MDictStuff__VuseDict)
  {
    fputs("\n#define HT_INIT_SIZE 16\n#define PERTURB_SHIFT 5\n\n#define CDI_FLAG_PVAL 1\n#define CDI_FLAG_USED 2\n#define CDI_FLAG_DEL 4\nstruct CDictItem__S {\n  Zhashtype hash;\n  union {\n    Zint Ikey;\n    void *Pkey;\n  };\n  union {\n    Zint Ival;\n    void *Pval;\n  };\n  int flags;\n};\n\nstruct CDictHead__S {\n    Zhashtype   mask;\n    Zhashtype   used;\n    Zhashtype   filled;\n    int         locked;\n    CDictItem   *array;\n    CDictItem   smallArray[HT_INIT_SIZE];\n    int         usePkey;\n};\n", Afd);
  }
}
void MDictStuff__FwriteBody(CScope *AtopScope, FILE *Afd) {
  if (MDictStuff__VuseDict)
  {
    fputs("\nCDictHead *ZnewDict(int usePkey) {\n  CDictHead *d = Zalloc(sizeof(CDictHead));\n  d->array = d->smallArray;\n  d->mask = HT_INIT_SIZE - 1;\n  d->usePkey = usePkey;\n  return d;\n}\n\nZhashtype ZDictHash(CDictHead *d, Zint Ikey, unsigned char *Pkey) {\n  if (d->usePkey) {\n    Zhashtype hash = *Pkey;\n    if (hash != 0) {\n      unsigned char *p = Pkey + 1;\n      while (*p != 0)\n        hash = hash * 101 + *p++;\n    }\n    return hash;\n  }\n  return (Zhashtype)Ikey;\n}\n\n/* #define DICT_DEBUG 1 */\n\nCDictItem *ZDictLookup(CDictHead *d, Zint Ikey, void *Pkey, Zhashtype hash)\n{\n  Zhashtype  perturb;\n  CDictItem  *freeitem;\n  CDictItem  *di;\n  int        idx;\n#ifdef DICT_DEBUG\n  int        echo = Pkey != NULL && strcmp(Pkey, \"EQUAL\") == 0;\n#endif\n\n  idx = (int)(hash & d->mask);\n  di = &d->array[idx];\n#ifdef DICT_DEBUG\n  if (echo)\n    printf(\"idx = %d, flags = %d, key = %s, value = %lld\\n\",\n    idx, di->flags, (char *)di->Pkey, di->Ival);\n#endif\n\n  if (di->flags == 0)\n    return di;\n  if (di->flags == CDI_FLAG_DEL)\n    freeitem = di;\n  /* TODO: other keys than string */\n  else if (di->hash == hash && (d->usePkey\n       ? (di->Pkey != NULL && Pkey != NULL && strcmp(di->Pkey, Pkey) == 0)\n       : di->Ikey == Ikey)) {\n#ifdef DICT_DEBUG\n    if (echo) printf(\"found it\\n\");\n#endif\n    return di;\n  } else {\n    freeitem = NULL;\n  }\n\n  for (perturb = hash; ; perturb >>= PERTURB_SHIFT)\n  {\n    idx = (int)((idx << 2) + idx + perturb + 1);\n    di = &d->array[idx & d->mask];\n#ifdef DICT_DEBUG\n    if (echo)\n      printf(\"idx = %d, flags = %d\\n\", idx, di->flags);\n#endif\n    if (di->flags == 0)\n      return freeitem == NULL ? di : freeitem;\n    if (di->hash == hash\n            && di->flags != CDI_FLAG_DEL\n            && (d->usePkey\n                 ? (di->Pkey != NULL && Pkey != NULL\n                                               && strcmp(di->Pkey, Pkey) == 0)\n                 : di->Ikey == Ikey))\n      return di;\n    if (di->flags == CDI_FLAG_DEL && freeitem == NULL)\n      freeitem = di;\n  }\n}\n\nvoid ZDictResize(CDictHead *d, int minitems) {\n  CDictItem  temparray[HT_INIT_SIZE];\n  CDictItem  *oldarray, *newarray;\n  CDictItem  *olditem, *newitem;\n  int        newi;\n  int        todo;\n  Zhashtype  oldsize, newsize;\n  Zhashtype  minsize;\n  Zhashtype  newmask;\n  Zhashtype  perturb;\n\n  if (d->locked > 0)\n    return;\n\n#ifdef DICT_DEBUG\n  printf(\"size: %lu, filled: %lu, used: %lu\\n\",\n           d->mask + 1, d->filled, d->used);\n#endif\n\n  if (minitems == 0) {\n    if (d->filled < HT_INIT_SIZE - 1 && d->array == d->smallArray) {\n#ifdef DICT_DEBUG\n      printf(\"small array not full\\n\");\n#endif\n      return;\n    }\n    oldsize = d->mask + 1;\n    if (d->filled * 3 < oldsize * 2 && d->used > oldsize / 5) {\n#ifdef DICT_DEBUG\n      printf(\"size OK\\n\");\n#endif\n      return;\n    }\n    if (d->used > 1000)\n      minsize = d->used * 2;\n    else\n      minsize = d->used * 4;\n  } else {\n    if ((Zhashtype)minitems < d->used)\n      minitems = (int)d->used;\n    minsize = minitems * 3 / 2;\n  }\n\n  newsize = HT_INIT_SIZE;\n  while (newsize < minsize) {\n    newsize <<= 1;\n    if (newsize == 0) {\n      /* TODO: throw exception */\n      fputs(\"EXCEPTION: ZDictResize\\n\", stdout);\n      return;\n    }\n  }\n\n#ifdef DICT_DEBUG\n  printf(\"resizing from %lu to %lu\\n\", d->mask + 1, newsize);\n#endif\n\n  if (newsize == HT_INIT_SIZE) {\n    newarray = d->smallArray;\n    if (d->array == newarray) {\n      memmove(temparray, newarray, sizeof(temparray));\n      oldarray = temparray;\n    } else\n      oldarray = d->array;\n    memset(newarray, 0, (size_t)(sizeof(CDictItem) * newsize));\n  } else {\n    newarray = (CDictItem *)Zalloc((sizeof(CDictItem) * newsize));\n    if (newarray == NULL) {\n      /* TODO: throw exception */\n      fputs(\"EXCEPTION: ZDictResize\\n\", stdout);\n      return;\n    }\n    oldarray = d->array;\n  }\n\n  newmask = newsize - 1;\n  todo = (int)d->used;\n  for (olditem = oldarray; todo > 0; ++olditem)\n    if (olditem->flags & CDI_FLAG_USED) {\n      newi = (int)(olditem->hash & newmask);\n      newitem = &newarray[newi];\n      if (newitem->flags != 0)\n        for (perturb = olditem->hash; ; perturb >>= PERTURB_SHIFT) {\n          newi = (int)((newi << 2) + newi + perturb + 1);\n          newitem = &newarray[newi & newmask];\n          if (newitem->flags == 0)\n            break;\n        }\n      *newitem = *olditem;\n      --todo;\n    }\n\n  if (d->array != d->smallArray)\n      free(d->array);\n  d->array = newarray;\n  d->mask = newmask;\n  d->filled = d->used;\n}\n\n/* \"ow\" is the overwrite flag.  When zero it's not allowed to overwrite an\nexisting entry. */\nCDictHead *ZDictAdd(int ow, CDictHead *d, Zint Ikey, void *Pkey, Zint Ivalue, void *Pvalue) {\n  Zhashtype  hash = ZDictHash(d, Ikey, Pkey);\n  CDictItem  *di = ZDictLookup(d, Ikey, Pkey, hash);\n#ifdef DICT_DEBUG\n  if (d->usePkey)\n    printf(\"Adding item %s\\n\", (char *)Pkey);\n  else\n    printf(\"Adding item %lld\\n\", Ikey);\n  if (Pkey != NULL\n      && (strcmp(Pkey, \"ENUM\") == 0\n          || strcmp(Pkey, \"EQUAL\") == 0\n          || strcmp(Pkey, \"EXIT\") == 0))\n    dumpdict(d);\n#endif\n  if (di->flags == 0 || di->flags == CDI_FLAG_DEL || ow) {\n    if (di->flags == 0 || di->flags == CDI_FLAG_DEL) {\n      ++d->used;\n      if (di->flags == 0)\n        ++d->filled;\n    }\n    di->hash = hash;\n    if (d->usePkey)\n      di->Pkey = Pkey;\n    else\n      di->Ikey = Ikey;\n    di->flags = CDI_FLAG_USED;\n\n    if (Pvalue == NULL) {\n      di->Ival = Ivalue;\n      di->flags &= ~CDI_FLAG_PVAL;\n    } else {\n      di->Pval = Pvalue;\n      di->flags |= CDI_FLAG_PVAL;\n    }\n\n    ZDictResize(d, 0);\n  } else {\n    /* TODO: throw exception? */\n    if (d->usePkey)\n      printf(\"EXCEPTION: ZDictAdd for %s\\n\", (char *)Pkey);\n    else\n      printf(\"EXCEPTION: ZDictAdd for %lld\\n\", Ikey);\n  }\n  return d;\n}\n\n#ifdef DICT_DEBUG\ndumpdict(CDictHead *d)\n{\n  int        todo = (int)d->used;\n  CDictItem  *item;\n  int        idx = 0;\n\n  for (item = d->array; todo > 0; ++item) {\n    if (item->flags & CDI_FLAG_USED) {\n      printf(\"%2d: %s\\n\", idx, (char *)item->Pkey);\n      --todo;\n    } else if (item->flags == 0) {\n      printf(\"%2d: unused\\n\", idx);\n    } else if (item->flags == CDI_FLAG_DEL) {\n      printf(\"%2d: deleted\\n\", idx);\n    } else {\n      printf(\"%2d: invalid flags: %d\\n\", idx, item->flags);\n    }\n    ++idx;\n  }\n}\n#endif\n\n", Afd);
  }
  if (MDictStuff__VuseZDictFind)
  {
    fputs("\nCDictItem *ZDictFind(CDictHead *d, Zint Ikey, void *Pkey) {\n  Zhashtype  hash = ZDictHash(d, Ikey, Pkey);\n  CDictItem  *di = ZDictLookup(d, Ikey, Pkey, hash);\n  if (di->flags & CDI_FLAG_USED)\n    return di;\n  return NULL;\n}\n", Afd);
  }
  if (MDictStuff__VuseZDictGetPtr)
  {
    fputs("\nvoid *ZDictGetPtr(CDictHead *d, Zint Ikey, void *Pkey) {\n  CDictItem *di = ZDictFind(d, Ikey, Pkey);\n  if (di != NULL)\n    return di->Pval;\n  fputs(\"EXCEPTION: ZDictGetPtr\\n\", stdout);\n  return NULL;  /* TODO: throw exception */\n}\n", Afd);
  }
  if (MDictStuff__VuseZDictGetPtrDef)
  {
    fputs("\nvoid *ZDictGetPtrDef(CDictHead *d, Zint Ikey, void *Pkey, void *def) {\n  CDictItem *di = ZDictFind(d, Ikey, Pkey);\n  if (di != NULL)\n    return di->Pval;\n  return def;\n}\n", Afd);
  }
  if (MDictStuff__VuseZDictGetInt)
  {
    fputs("\nZint ZDictGetInt(CDictHead *d, Zint Ikey, void *Pkey) {\n  CDictItem *di = ZDictFind(d, Ikey, Pkey);\n  if (di != NULL)\n    return di->Ival;\n  fputs(\"EXCEPTION: ZDictGetInt\\n\", stdout);\n  return 0;  /* TODO: throw exception */\n}\n", Afd);
  }
  if (MDictStuff__VuseZDictGetIntDef)
  {
    fputs("\nZint ZDictGetIntDef(CDictHead *d, Zint Ikey, void *Pkey, Zint def) {\n  CDictItem *di = ZDictFind(d, Ikey, Pkey);\n  if (di != NULL)\n    return di->Ival;\n  return def;\n}\n", Afd);
  }
  if (MDictStuff__VuseZDictHas)
  {
    fputs("\nZbool ZDictHas(CDictHead *d, Zint Ikey, void *Pkey) {\n  return (ZDictFind(d, Ikey, Pkey) != NULL);\n}\n", Afd);
  }
  if (MDictStuff__VuseZDictRemove)
  {
    fputs("\nCDictHead *ZDictRemove(CDictHead *d, Zint Ikey, void *Pkey) {\n  CDictItem *di = ZDictFind(d, Ikey, Pkey);\n  if (di != NULL) {\n    di->flags = CDI_FLAG_DEL;\n    --d->used;\n    ZDictResize(d, 0);\n  } else {\n    /* TODO: throw exception? */\n    fputs(\"EXCEPTION: ZDictRemove\\n\", stdout);\n  }\n  return d;\n}\n\n", Afd);
  }
  if (MDictStuff__VuseZDictToString)
  {
    fputs("\nvoid *ZDictToString(CDictHead *d) {\n  garray_T *ga = Zalloc(sizeof(garray_T));\n  int hash;\n  int first = 1;\n  int todo;\n  CDictItem *di;\n  ga_append(ga, \"{\");\n  todo = d->used;\n  for (di = d->array; todo > 0; ++di) {\n    if (di->flags & CDI_FLAG_USED) {\n      --todo;\n      if (first == 0) ga_append(ga, \", \"); else first = 0;\n      if (d->usePkey) {\n        ga_append(ga, \"\\\"\");\n        ga_append(ga, di->Pkey);\n        ga_append(ga, \"\\\"\");\n      } else\n        ga_append(ga, Zint2string(di->Ikey));\n      ga_append(ga, \" : \");\n      if (di->flags & CDI_FLAG_PVAL) {\n        ga_append(ga, \"\\\"\");\n        ga_append(ga, di->Pval);\n        ga_append(ga, \"\\\"\");\n      } else\n        ga_append(ga, Zint2string(di->Ival));\n    }\n  }\n  ga_append(ga, \"}\");\n  return ga->data;\n}\n", Afd);
  }
  if (MDictStuff__VuseZDictKeys)
  {
    fputs("\nCListHead *ZDictKeys(CDictHead *d) {\n  CListHead *l = Zalloc(sizeof(CListHead));\n  int hash;\n  int first = 1;\n  int todo;\n  CDictItem *di;\n  todo = d->used;\n  for (di = d->array; todo > 0; ++di) {\n    if (di->flags & CDI_FLAG_USED) {\n      --todo;\n      if (d->usePkey)\n        ZListAdd(l, -1, 0, di->Pkey, 1);\n      else\n        ZListAdd(l, -1, di->Ikey, NULL, 0);\n    }\n  }\n  return l;\n}\n", Afd);
  }
  if (MDictStuff__VuseZDictClear)
  {
    fputs("\nCDictHead *ZDictClear(CDictHead *d) {\n  if (d->array != d->smallArray)\n    free(d->array);\n  memset(d, 0, sizeof(CDictHead));\n  d->array = d->smallArray;\n  d->mask = HT_INIT_SIZE - 1;\n  return d;\n}\n", Afd);
  }
  if (MDictStuff__VuseZDictCopy)
  {
    fputs("\nCDictHead *ZDictCopy(CDictHead *d) {\n  CDictItem *di;\n  CDictHead *newd = malloc(sizeof(CDictHead));\n  memcpy(newd, d, sizeof(CDictHead));\n  if (d->array != d->smallArray) {\n    size_t len = (d->mask + 1) * sizeof(CDictItem);\n    newd->array = malloc(len);\n    memcpy(newd->array, d->array, len);\n  }\n  return newd;\n}\n", Afd);
  }
}
void Idictstuff() {
 static int done = 0;
 if (!done) {
  done = 1;
  Igenerate();
  Iliststuff();
  Ioutput();
  Iscontext();
  Iscope();
  Isymbol();
  MDictStuff__VuseDict = 0;
  MDictStuff__VuseZDictHas = 0;
  MDictStuff__VuseZDictRemove = 0;
  MDictStuff__VuseZDictFind = 0;
  MDictStuff__VuseZDictGetPtr = 0;
  MDictStuff__VuseZDictGetPtrDef = 0;
  MDictStuff__VuseZDictGetInt = 0;
  MDictStuff__VuseZDictGetIntDef = 0;
  MDictStuff__VuseZDictToString = 0;
  MDictStuff__VuseZDictKeys = 0;
  MDictStuff__VuseZDictCopy = 0;
  MDictStuff__VuseZDictClear = 0;
 }
}
