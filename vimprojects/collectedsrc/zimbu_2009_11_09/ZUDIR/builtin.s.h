#define INC_builtin_S 1
/*
 * STRUCTS
 */
#ifndef INC_config_S
#include "../ZUDIR/config.s.h"
#endif
#ifndef INC_error_S
#include "../ZUDIR/error.s.h"
#endif
#ifndef INC_generate_S
#include "../ZUDIR/generate.s.h"
#endif
#ifndef INC_output_S
#include "../ZUDIR/output.s.h"
#endif
#ifndef INC_parse_S
#include "../ZUDIR/parse.s.h"
#endif
#ifndef INC_scope_S
#include "../ZUDIR/scope.s.h"
#endif
#ifndef INC_resolve_S
#include "../ZUDIR/resolve.s.h"
#endif
#ifndef INC_symbol_S
#include "../ZUDIR/symbol.s.h"
#endif
#ifndef INC_tokenize_S
#include "../ZUDIR/tokenize.s.h"
#endif
#ifndef INC_zimbufile_S
#include "../ZUDIR/zimbufile.s.h"
#endif
#ifndef INC_usedfile_S
#include "../ZUDIR/usedfile.s.h"
#endif
#ifndef INC_httploader_S
#include "../lib/ZUDIR/httploader.s.h"
#endif
#ifndef INC_infoloader_S
#include "../lib/ZUDIR/infoloader.s.h"
#endif
#ifndef INC_zwtloader_S
#include "../lib/ZUDIR/zwtloader.s.h"
#endif
struct CBuiltin__S {
  char *VmoduleName;
  char *VfileName;
  CUsedFile *VusedFile;
  void *Vsetup;
};
