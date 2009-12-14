#define INC_builtin_B 1
/*
 * FUNCTION BODIES
 */
#ifndef INC_config_B
#include "../ZUDIR/config.b.c"
#endif
#ifndef INC_error_B
#include "../ZUDIR/error.b.c"
#endif
#ifndef INC_generate_B
#include "../ZUDIR/generate.b.c"
#endif
#ifndef INC_output_B
#include "../ZUDIR/output.b.c"
#endif
#ifndef INC_parse_B
#include "../ZUDIR/parse.b.c"
#endif
#ifndef INC_scope_B
#include "../ZUDIR/scope.b.c"
#endif
#ifndef INC_resolve_B
#include "../ZUDIR/resolve.b.c"
#endif
#ifndef INC_symbol_B
#include "../ZUDIR/symbol.b.c"
#endif
#ifndef INC_tokenize_B
#include "../ZUDIR/tokenize.b.c"
#endif
#ifndef INC_zimbufile_B
#include "../ZUDIR/zimbufile.b.c"
#endif
#ifndef INC_usedfile_B
#include "../ZUDIR/usedfile.b.c"
#endif
#ifndef INC_httploader_B
#include "../lib/ZUDIR/httploader.b.c"
#endif
#ifndef INC_infoloader_B
#include "../lib/ZUDIR/infoloader.b.c"
#endif
#ifndef INC_zwtloader_B
#include "../lib/ZUDIR/zwtloader.b.c"
#endif
void CBuiltin__FaddAllMembers(CBuiltin *THIS, CSymbol *AmoduleSymbol) {
  CScope *Vscope;
  Vscope = THIS->VusedFile->VzimbuFile->VtopScope;
  if (((Vscope != NULL) && (Vscope->VmemberList != NULL)))
  {
    if ((Vscope->VmemberList->itemCount != 1))
    {
      MError__Freport(Zconcat(Zconcat(Zconcat("INTERNAL: found ", Zint2string(Vscope->VmemberList->itemCount)), " symbols in "), THIS->VfileName));
    }
  else
    {
      CSymbol *Vsym;
      Vsym = ((CSymbol *)ZListGetPtr(Vscope->VmemberList, 0));
      if ((Vsym->VmemberList != NULL))
      {
        {
          Zfor_T *Zf = ZforNew(Vsym->VmemberList, 2);
          CSymbol *Vm;
          for (ZforGetPtr(Zf, (char **)&Vm); ZforCont(Zf); ZforNextPtr(Zf, (char **)&Vm)) {
            MError__Fverbose2Msg(Zconcat(Zconcat(Zconcat(Zconcat("Adding ", AmoduleSymbol->Vname), "."), Vm->Vname), "\n"));
            CSymbol__FaddMember__1(AmoduleSymbol, Vm);
          }
        }
      }
    }
  }
}
void CBuiltin__X__Fprepare() {
  CBuiltin__X__VavailableModules = ZnewDict(1);
  ZDictAdd(1, CBuiltin__X__VavailableModules, 0, "HTTP", 0, MHTTPloader__Fprepare());
  ZDictAdd(1, CBuiltin__X__VavailableModules, 0, "ZWT", 0, MZWTloader__Fprepare());
  ZDictAdd(1, CBuiltin__X__VavailableModules, 0, "INFO", 0, MINFOloader__Fprepare());
}
void CBuiltin__X__FcheckBuiltin(CUsedFile *A_usedFile) {
  CUsedFile *VmainFile;
  VmainFile = CUsedFile__FfindTopFile(A_usedFile);
  if ((VmainFile != NULL))
  {
    CScope *Vscope;
    Vscope = A_usedFile->VzimbuFile->VtopScope;
    if ((Vscope->VusedIdKeywords != NULL))
    {
      {
        Zfor_T *Zf = ZforNew(ZDictKeys(Vscope->VusedIdKeywords), 2);
        char *Vid;
        for (ZforGetPtr(Zf, (char **)&Vid); ZforCont(Zf); ZforNextPtr(Zf, (char **)&Vid)) {
          if (!(ZDictHas(VmainFile->VusedBuiltins, 0, Vid)))
          {
            ZDictAdd(1, VmainFile->VusedBuiltins, 0, Vid, 0, ZDictGetPtrDef(CBuiltin__X__VavailableModules, 0, Vid, NULL));
          }
        }
      }
    }
  }
}
Zint CBuiltin__X__FgenerateBuiltins(CUsedFile *AusedFile, Zoref *Agen, COutput__X__CGroup *Aouts) {
  CScope *Vscope;
  Vscope = CUsedFile__Fscope(AusedFile);
  Zint Vundef;
  Vundef = 0;
  if ((AusedFile->VusedBuiltins != NULL))
  {
    {
      Zfor_T *Zf = ZforNew(ZDictKeys(AusedFile->VusedBuiltins), 2);
      char *Vid;
      for (ZforGetPtr(Zf, (char **)&Vid); ZforCont(Zf); ZforNextPtr(Zf, (char **)&Vid)) {
        CBuiltin *Vbuiltin;
        Vbuiltin = ZDictGetPtr(AusedFile->VusedBuiltins, 0, Vid);
        if ((Vbuiltin != NULL))
        {
          CZimbuFile *VzimbuFile;
          VzimbuFile = NULL;
          if ((Vbuiltin->VusedFile != NULL))
          {
            VzimbuFile = Vbuiltin->VusedFile->VzimbuFile;
          }
        else
          {
            char *Vname;
            Vname = Zconcat(Zconcat(MConfig__VlibPath, "/"), Vbuiltin->VfileName);
            VzimbuFile = CZimbuFile__FNEW(Vname);
            Vbuiltin->VusedFile = CUsedFile__FNEW__1(VzimbuFile, 0, 0);
            if ((CUsedFile__Fparse(Vbuiltin->VusedFile, "  ") == 0))
            {
              MError__Freport(Zconcat("Cannot open file for reading: ", Vname));
            }
          else if ((VzimbuFile->VtopScope->VtopNode != NULL))
            {
              VzimbuFile->VtopScope->VtopNode->Vn_undefined = 99;
            }
            VzimbuFile->VdirName = MConfig__VlibPath;
          }
          VzimbuFile->VusedAsZimbu = AusedFile->VzimbuFile->VusedAsZimbu;
          VzimbuFile->VusedAsZwt = AusedFile->VzimbuFile->VusedAsZwt;
          if ((Vbuiltin->VusedFile != NULL))
          {
            Vundef += MGenerate__FprocessImport(Vbuiltin->VusedFile, Agen, Vscope, Aouts);
            if ((Vbuiltin->Vsetup != NULL))
            {
              ((void (*)(CBuiltin *))Vbuiltin->Vsetup)(Vbuiltin);
            }
          }
        }
      }
    }
  }
  return Vundef;
}
void Ibuiltin() {
 static int done = 0;
 if (!done) {
  done = 1;
  Iconfig();
  Ierror();
  Igenerate();
  Ioutput();
  Iparse();
  Iscope();
  Iresolve();
  Isymbol();
  Itokenize();
  Izimbufile();
  Iusedfile();
  Ihttploader();
  Iinfoloader();
  Izwtloader();
  CBuiltin__X__VavailableModules = NULL;
 }
}
