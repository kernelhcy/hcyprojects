#define INC_builtin_D 1
/*
 * DECLARE FUNCTIONS AND GLOBALS
 */
#ifndef INC_config_D
#include "../ZUDIR/config.d.h"
#endif
#ifndef INC_error_D
#include "../ZUDIR/error.d.h"
#endif
#ifndef INC_generate_D
#include "../ZUDIR/generate.d.h"
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
#ifndef INC_resolve_D
#include "../ZUDIR/resolve.d.h"
#endif
#ifndef INC_symbol_D
#include "../ZUDIR/symbol.d.h"
#endif
#ifndef INC_tokenize_D
#include "../ZUDIR/tokenize.d.h"
#endif
#ifndef INC_zimbufile_D
#include "../ZUDIR/zimbufile.d.h"
#endif
#ifndef INC_usedfile_D
#include "../ZUDIR/usedfile.d.h"
#endif
#ifndef INC_httploader_D
#include "../lib/ZUDIR/httploader.d.h"
#endif
#ifndef INC_infoloader_D
#include "../lib/ZUDIR/infoloader.d.h"
#endif
#ifndef INC_zwtloader_D
#include "../lib/ZUDIR/zwtloader.d.h"
#endif
void CBuiltin__FaddAllMembers(CBuiltin *THIS, CSymbol *AmoduleSymbol);
CDictHead *CBuiltin__X__VavailableModules;
void CBuiltin__X__Fprepare();
void CBuiltin__X__FcheckBuiltin(CUsedFile *A_usedFile);
Zint CBuiltin__X__FgenerateBuiltins(CUsedFile *AusedFile, Zoref *Agen, COutput__X__CGroup *Aouts);
void Ibuiltin();
