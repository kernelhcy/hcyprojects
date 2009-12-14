#define INC_config_B 1
/*
 * FUNCTION BODIES
 */
Zstatus MConfig__Frun() {
  MConfig__Vint64name = "long long";
  MConfig__Vint32name = "int";
  Zint Vsz;
  Vsz = 0;
  Vsz = sizeof(int);
  MConfig__VintSize = Vsz;
  char *Vsuf;
  Vsuf = "";
  char *Vtarg;
  Vtarg = "-lpthread";
#if defined(__MINGW32__) || defined(_MSC_VER)
  Vsuf = ".exe";  /* set to ".exe" for MS-Windows */
#endif
#if defined(__MINGW32__)
  Vtarg = "-lpthreadGC2";  /* suggested by edcdave, requires pthreads-win32 */
#endif
  MConfig__VexeSuffix = Vsuf;
  MConfig__VthreadArg = Vtarg;
  Zint Vi;
  Vi = 0;
  char *Vroot;
  Vroot = NULL;
  Vi = ZStringRindex(MARG__Vname, 47);
  if ((Vi < 0))
  {
    Vi = ZStringRindex(MARG__Vname, 92);
  }
  if ((Vi < 0))
  {
    Vroot = "";
  }
else
  {
    Vroot = ZStringSlice(MARG__Vname, 0, Vi);
  }
  MConfig__VlibPath = Zconcat(Vroot, "lib");
  return 1;
}
void MConfig__FaddThreadLib() {
  ZListAdd(MConfig__VlibArgs, -1, 0, MConfig__VthreadArg, 1);
}
char *MConfig__FcompilerCmd(char *AsrcName, char *AbinName) {
  char *Vcmd;
  Vcmd = Zconcat(Zconcat(Zconcat(MConfig__Vcompiler, " -g -o \""), AbinName), "\" ");
  {
    Zfor_T *Zf = ZforNew(MConfig__VlibArgs, 2);
    char *Vl;
    for (ZforGetPtr(Zf, (char **)&Vl); ZforCont(Zf); ZforNextPtr(Zf, (char **)&Vl)) {
      Vcmd = Zconcat(Vcmd, Zconcat(Vl, " "));
    }
  }
  return Zconcat(Zconcat(Zconcat(Vcmd, "\""), AsrcName), "\"");
}
void Iconfig() {
 static int done = 0;
 if (!done) {
  done = 1;
  MConfig__Vint64name = NULL;
  MConfig__Vint32name = NULL;
  MConfig__VintSize = 0;
  MConfig__VlibArgs = Zalloc(sizeof(CListHead));
  MConfig__VexeSuffix = NULL;
  MConfig__Vcompiler = "cc";
  MConfig__VthreadArg = NULL;
  MConfig__VlibPath = NULL;
  MConfig__VzudirName = "ZUDIR";
 }
}
