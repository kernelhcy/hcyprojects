#define INC_zimbufile_D 1
/*
 * DECLARE FUNCTIONS AND GLOBALS
 */
#ifndef INC_error_D
#include "../ZUDIR/error.d.h"
#endif
#ifndef INC_node_D
#include "../ZUDIR/node.d.h"
#endif
#ifndef INC_output_D
#include "../ZUDIR/output.d.h"
#endif
#ifndef INC_parse_D
#include "../ZUDIR/parse.d.h"
#endif
#ifndef INC_scope_D
#include "../ZUDIR/scope.d.h"
#endif
#ifndef INC_symbol_D
#include "../ZUDIR/symbol.d.h"
#endif
#ifndef INC_usedfile_D
#include "../ZUDIR/usedfile.d.h"
#endif
CZimbuFile *CZimbuFile__FNEW(char *A_filename);
Zstatus CZimbuFile__Fparse(CZimbuFile *THIS, char *Aindent, CUsedFile *AusedFile);
char *CZimbuFile__FgetModuleName(CZimbuFile *THIS);
CNode *CZimbuFile__FgetModuleNode(CZimbuFile *THIS);
CZimbuFile__X__CCodeSpecific *CZimbuFile__X__CCodeSpecific__FNEW__1();
void CZimbuFile__X__CCodeSpecific__Fclear(CZimbuFile__X__CCodeSpecific *THIS);
CZimbuFile *CZimbuFile__X__Ffind(CListHead *AimportList, char *Aname);
void Izimbufile();
