#define INC_output_D 1
/*
 * DECLARE FUNCTIONS AND GLOBALS
 */
#ifndef INC_scope_D
#include "../ZUDIR/scope.d.h"
#endif
COutput *COutput__FNEW(COutput__X__CFragmentHead *Ahead);
void COutput__Fwrite(COutput *THIS, char *As);
void COutput__Fprepend(COutput *THIS, char *As);
void COutput__Fappend(COutput *THIS, COutput *Aout);
void COutput__FwriteIndent(COutput *THIS, Zint Adepth);
char *COutput__FtoString(COutput *THIS);
COutput *COutput__Fcopy(COutput *THIS);
void COutput__Fclear(COutput *THIS);
void COutput__Freset(COutput *THIS, COutput *Asrc);
COutput *COutput__X__VnoOut;
COutput__X__CFragmentHead *COutput__X__CFragmentHead__FNEW__1();
void COutput__X__CFragmentHead__Fadd(COutput__X__CFragmentHead *THIS, char *Atext);
void COutput__X__CFragmentHead__Fappend__1(COutput__X__CFragmentHead *THIS, COutput__X__CFragmentHead *Ahead);
void COutput__X__CFragmentHead__Finsert(COutput__X__CFragmentHead *THIS, char *Atext);
COutput__X__CFragmentHead *COutput__X__CFragmentHead__Fcopy__1(COutput__X__CFragmentHead *THIS);
void COutput__X__CFragmentHead__Fwrite__1(COutput__X__CFragmentHead *THIS, FILE *Afd);
void COutput__X__CFragmentHead__Fclear__1(COutput__X__CFragmentHead *THIS);
char *COutput__X__CFragmentHead__FtoString__1(COutput__X__CFragmentHead *THIS);
Zbool COutput__X__CFragmentHead__Fempty(COutput__X__CFragmentHead *THIS);
COutput__X__CHeads *COutput__X__CHeads__FNEW__1();
COutput__X__CGroup *COutput__X__CGroup__FNEW__1();
COutput__X__CGroup *COutput__X__CGroup__FNEW__2(COutput__X__CHeads *Aheads);
void COutput__X__CGroup__FsetHeads(COutput__X__CGroup *THIS, COutput__X__CHeads *Aheads);
void COutput__X__CGroup__FstartWriting(COutput__X__CGroup *THIS);
COutput__X__CGroup *COutput__X__CGroup__Fcopy__1(COutput__X__CGroup *THIS);
void Ioutput();
