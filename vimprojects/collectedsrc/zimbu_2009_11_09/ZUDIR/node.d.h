#define INC_node_D 1
/*
 * DECLARE FUNCTIONS AND GLOBALS
 */
#ifndef INC_attr_D
#include "../ZUDIR/attr.d.h"
#endif
#ifndef INC_conversion_D
#include "../ZUDIR/conversion.d.h"
#endif
#ifndef INC_error_D
#include "../ZUDIR/error.d.h"
#endif
#ifndef INC_token_D
#include "../ZUDIR/token.d.h"
#endif
#ifndef INC_pos_D
#include "../ZUDIR/pos.d.h"
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
CNode *CNode__FNEW(Zenum Atype);
Zenum CNode__FresultType(CNode *THIS, CScope *Ascope);
CSymbol *CNode__FfindTopModule(CNode *THIS, CScope *Ascope);
void CNode__Ferror(CNode *THIS, char *Amsg);
char *CNode__FtoString(CNode *THIS);
char *CNode__FtypeString(CNode *THIS);
char *CNode__FtoString__1(CNode *THIS, char *Aindent, Zbool Arecurse, Zbool AdoNext);
void CNode__FcheckTypeName(CNode *THIS, char *Awhat);
void CNode__FcheckItemName(CNode *THIS, char *Awhat);
Zenum CNode__X__Fname2Type(char *Aname);
Zbool CNode__X__FisPointerType(Zenum Atype);
Zbool CNode__X__FisMethodType(Zenum Atype);
Zbool CNode__X__FisTypeWithArgs(Zenum Atype);
void Inode();
