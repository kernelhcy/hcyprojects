#define INC_use_B 1
/*
 * FUNCTION BODIES
 */
#ifndef INC_config_B
#include "../ZUDIR/config.b.c"
#endif
#ifndef INC_dictstuff_B
#include "../ZUDIR/dictstuff.b.c"
#endif
#ifndef INC_liststuff_B
#include "../ZUDIR/liststuff.b.c"
#endif
#ifndef INC_output_B
#include "../ZUDIR/output.b.c"
#endif
#ifndef INC_scope_B
#include "../ZUDIR/scope.b.c"
#endif
void Iuse() {
 static int done = 0;
 if (!done) {
  done = 1;
  Iconfig();
  Idictstuff();
  Iliststuff();
  Iscope();
 }
}
