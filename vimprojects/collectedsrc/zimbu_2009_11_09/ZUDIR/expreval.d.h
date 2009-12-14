#define INC_expreval_D 1
/*
 * DECLARE FUNCTIONS AND GLOBALS
 */
#ifndef INC_node_D
#include "../ZUDIR/node.d.h"
#endif
#ifndef INC_scontext_D
#include "../ZUDIR/scontext.d.h"
#endif
Zbool MExprEval__FevalBool(CNode *Anode, CSContext *Actx);
char *MExprEval__FevalString(CNode *Anode, CSContext *Actx);
void Iexpreval();
