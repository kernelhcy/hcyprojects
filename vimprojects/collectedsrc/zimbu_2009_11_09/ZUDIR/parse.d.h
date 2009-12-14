#define INC_parse_D 1
/*
 * DECLARE FUNCTIONS AND GLOBALS
 */
#ifndef INC_attr_D
#include "../ZUDIR/attr.d.h"
#endif
#ifndef INC_error_D
#include "../ZUDIR/error.d.h"
#endif
#ifndef INC_input_D
#include "../ZUDIR/input.d.h"
#endif
#ifndef INC_node_D
#include "../ZUDIR/node.d.h"
#endif
#ifndef INC_pos_D
#include "../ZUDIR/pos.d.h"
#endif
#ifndef INC_scope_D
#include "../ZUDIR/scope.d.h"
#endif
#ifndef INC_token_D
#include "../ZUDIR/token.d.h"
#endif
#ifndef INC_tokenize_D
#include "../ZUDIR/tokenize.d.h"
#endif
#ifndef INC_usedfile_D
#include "../ZUDIR/usedfile.d.h"
#endif
CScope *MParse__FparseFile(char *AfileName, char *Aindent, CUsedFile *AusedFile);
CNode *MParse__FparseInput(CInput *Ain, Zbool AallowMain);
CNode *MParse__FnewNode(CNode *Aend_node, CToken *Atoken);
CNode *MParse__FparseBlock(CInput *Ain, Zbits AblockType);
CNode *MParse__FparseBlockItem(CInput *Ain, CNode *Astart_node, Zbits AblockType);
CNode *MParse__FparseClass(CToken *AclassToken, Zbbits Aattr, CNode *Anode_in, CInput *Ain);
CNode *MParse__FparseMethod(CToken *AfirstToken, Zbbits Aattr, CNode *Anode_in, CInput *Ain, Zbits AblockType);
CNode *MParse__FparseExpr(CInput *Ain);
CNode *MParse__FparseRefExpr(CInput *Ain);
CNode *MParse__FparseExprAlt(CInput *Ain);
CNode *MParse__FparseExprOr(CInput *Ain);
CNode *MParse__FparseExprAnd(CInput *Ain);
CNode *MParse__FparseExprComp(CInput *Ain);
CNode *MParse__FparseExprConcat(CInput *Ain);
CNode *MParse__FparseExprBitwise(CInput *Ain);
CNode *MParse__FparseExprShift(CInput *Ain);
CNode *MParse__FparseExprPlus(CInput *Ain);
CNode *MParse__FparseExprMult(CInput *Ain);
CNode *MParse__FparseExprIndecr(CInput *Ain);
CNode *MParse__FparseExprNeg(CInput *Ain);
CNode *MParse__FparseExprDot(CInput *Ain);
CNode *MParse__FparseExprParen(CInput *Ain);
CNode *MParse__FparseExprBase(CInput *Ain);
CNode *MParse__FparseId(CToken *Atoken);
void MParse__FparseNew(CNode *Anode, CInput *Ain);
CNode *MParse__FparseList(CInput *Ain);
CNode *MParse__FparseDict(CInput *Ain);
CNode *MParse__FnewOp(CNode *Anode, CInput *Ain);
CNode *MParse__FparseDotName(CInput *Ain);
CNode *MParse__FparseTypeName(CInput *Ain);
CNode *MParse__FparseNameList(CInput *Ain);
CNode *MParse__FparseComma(CInput *Ain, Zbits Aflags);
CNode *MParse__FparseMembers(CNode *Astart_node, CInput *Ain, Zbool AdoTypespec);
CNode *MParse__FcopyCode(CInput *Ain);
void MParse__Ferror(char *Amsg, CInput *Ain);
void MParse__FexpectLineSep(CInput *Ain);
void MParse__FexpectNewLine(CInput *Ain);
void MParse__FexpectLineSep__1(CInput *Ain, Zbool AallowSemicolon);
void MParse__FexpectSep(CInput *Ain);
void MParse__FskipSep(CInput *Ain);
void MParse__FskipLineSep(CInput *Ain);
CToken *MParse__FtokenAfterSep(CInput *Ain);
void MParse__FcheckNoSep(CInput *Ain);
void Iparse();
