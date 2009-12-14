#define INC_config_D 1
/*
 * DECLARE FUNCTIONS AND GLOBALS
 */
char *MConfig__Vint64name;
char *MConfig__Vint32name;
Zint MConfig__VintSize;
CListHead *MConfig__VlibArgs;
char *MConfig__VexeSuffix;
char *MConfig__Vcompiler;
char *MConfig__VthreadArg;
char *MConfig__VlibPath;
char *MConfig__VzudirName;
Zstatus MConfig__Frun();
void MConfig__FaddThreadLib();
char *MConfig__FcompilerCmd(char *AsrcName, char *AbinName);
void Iconfig();
