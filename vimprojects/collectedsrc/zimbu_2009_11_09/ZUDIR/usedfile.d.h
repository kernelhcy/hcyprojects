#define INC_usedfile_D 1
/*
 * DECLARE FUNCTIONS AND GLOBALS
 */
#ifndef INC_builtin_D
#include "../ZUDIR/builtin.d.h"
#endif
#ifndef INC_scope_D
#include "../ZUDIR/scope.d.h"
#endif
#ifndef INC_zimbufile_D
#include "../ZUDIR/zimbufile.d.h"
#endif
CUsedFile *CUsedFile__FNEW(char *AfileName, Zbits Aflags);
CUsedFile *CUsedFile__FNEW__1(CZimbuFile *A_zimbuFile, Zbool A_isMainFile, Zbool A_isTopFile);
Zstatus CUsedFile__Fparse(CUsedFile *THIS, char *Aindent);
CScope *CUsedFile__Fscope(CUsedFile *THIS);
CUsedFile *CUsedFile__FfindTopFile(CUsedFile *THIS);
Zbool CUsedFile__FisNewImport(CUsedFile *THIS, CZimbuFile *Azf);
void Iusedfile();
