#define INC_error_D 1
/*
 * DECLARE FUNCTIONS AND GLOBALS
 */
#ifndef INC_node_D
#include "../ZUDIR/node.d.h"
#endif
#ifndef INC_pos_D
#include "../ZUDIR/pos.d.h"
#endif
Zbool MError__VfoundError;
Zint MError__VerrorCount;
Zint MError__Vverbose;
Zbool MError__Vdebug;
void MError__Freport(char *Amsg);
void MError__Freport__1(char *Amsg, CPos *Apos);
void MError__FverboseMsg(char *Amsg);
void MError__Fverbose2Msg(char *Amsg);
Zstatus MError__FcheckArgCount(CNode *Anode, Zint Amin, Zint Amax, char *Aname);
Zbool MError__FfindDeepUndef(CNode *Anode);
void Ierror();
