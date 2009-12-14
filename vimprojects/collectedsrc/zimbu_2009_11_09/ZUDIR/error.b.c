#define INC_error_B 1
/*
 * FUNCTION BODIES
 */
#ifndef INC_node_B
#include "../ZUDIR/node.b.c"
#endif
#ifndef INC_pos_B
#include "../ZUDIR/pos.b.c"
#endif
void MError__Freport(char *Amsg) {
  MError__Freport__1(Amsg, NULL);
}
void MError__Freport__1(char *Amsg, CPos *Apos) {
  MError__VfoundError = 1;
  ++(MError__VerrorCount);
  if ((Apos == NULL))
  {
    (fputs(Zconcat("ERROR: ", Amsg), stdout) | fputc('\n', stdout));
  }
else
  {
    fputs(CPos__FtoString(Apos), stdout);
    (fputs(Zconcat(": ERROR: ", Amsg), stdout) | fputc('\n', stdout));
  }
  fflush(stdout);
}
void MError__FverboseMsg(char *Amsg) {
  if ((MError__Vverbose >= 1))
  {
    fputs(Amsg, stdout);
    fflush(stdout);
  }
}
void MError__Fverbose2Msg(char *Amsg) {
  if ((MError__Vverbose >= 2))
  {
    fputs(Amsg, stdout);
    fflush(stdout);
  }
}
Zstatus MError__FcheckArgCount(CNode *Anode, Zint Amin, Zint Amax, char *Aname) {
  Zint VargCount;
  VargCount = 0;
  if (((Anode != NULL) && (Anode->Vn_type != 0)))
  {
    ++(VargCount);
    CNode *Vn;
    Vn = Anode;
    while ((Vn->Vn_type == 110))
    {
      ++(VargCount);
      Vn = Vn->Vn_left;
    }
  }
  if ((VargCount > Amax))
  {
    if ((Amax == 0))
    {
      MError__Freport__1(Zconcat(Aname, "() does not accept arguments"), Anode->Vn_start);
    }
  else if ((Amax == 1))
    {
      MError__Freport__1(Zconcat(Zconcat(Aname, "() takes only 1 argument, found "), Zint2string(VargCount)), Anode->Vn_start);
    }
  else
    {
      MError__Freport__1(Zconcat(Zconcat(Zconcat(Zconcat(Aname, "() takes up to "), Zint2string(Amax)), " arguments, found "), Zint2string(VargCount)), Anode->Vn_start);
    }
    return 0;
  }
else if ((VargCount < Amin))
  {
    char *Vmsg;
    Vmsg = Zconcat(Zconcat(Aname, "() requires at least "), Zint2string(Amin));
    if ((Amin == 1))
    {
      Vmsg = Zconcat(Vmsg, " argument");
    }
  else
    {
      Vmsg = Zconcat(Vmsg, " arguments");
    }
    if ((VargCount == 0))
    {
      MError__Freport__1(Vmsg, Anode->Vn_start);
    }
  else
    {
      MError__Freport__1(Zconcat(Zconcat(Vmsg, ", found only "), Zint2string(VargCount)), Anode->Vn_start);
    }
    return 0;
  }
  return 1;
}
Zbool MError__FfindDeepUndef(CNode *Anode) {
  Zbool Vdone;
  Vdone = 0;
  if ((Anode->Vn_next != NULL))
  {
    Vdone = (Vdone || MError__FfindDeepUndef(Anode->Vn_next));
  }
  if (((Anode->Vn_left != NULL) && (Anode->Vn_left->Vn_undefined > 0)))
  {
    Vdone = (Vdone || MError__FfindDeepUndef(Anode->Vn_left));
  }
  if (((Anode->Vn_right != NULL) && (Anode->Vn_right->Vn_undefined > 0)))
  {
    Vdone = (Vdone || MError__FfindDeepUndef(Anode->Vn_right));
  }
  if (((Anode->Vn_cond != NULL) && (Anode->Vn_cond->Vn_undefined > 0)))
  {
    Vdone = (Vdone || MError__FfindDeepUndef(Anode->Vn_cond));
  }
  if (((Anode->Vn_returnType != NULL) && (Anode->Vn_returnType->Vn_undefined > 0)))
  {
    Vdone = (Vdone || MError__FfindDeepUndef(Anode->Vn_returnType));
  }
  if ((!(Vdone) && (Anode->Vn_undefined > 0)))
  {
    (fputs(Zconcat("deepest undef: ", CNode__FtoString(Anode)), stdout) | fputc('\n', stdout));
    Vdone = 1;
  }
  return Vdone;
}
void Ierror() {
 static int done = 0;
 if (!done) {
  done = 1;
  Inode();
  MError__VfoundError = 0;
  MError__VerrorCount = 0;
  MError__Vverbose = 0;
  MError__Vdebug = 0;
 }
}
