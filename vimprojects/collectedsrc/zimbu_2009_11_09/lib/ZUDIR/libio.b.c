#define INC_libio_B 1
/*
 * FUNCTION BODIES
 */
#ifndef INC_generate_B
#include "../../ZUDIR/generate.b.c"
#endif
#ifndef INC_node_B
#include "../../ZUDIR/node.b.c"
#endif
#ifndef INC_output_B
#include "../../ZUDIR/output.b.c"
#endif
#ifndef INC_scontext_B
#include "../../ZUDIR/scontext.b.c"
#endif
#ifndef INC_scope_B
#include "../../ZUDIR/scope.b.c"
#endif
#ifndef INC_symbol_B
#include "../../ZUDIR/symbol.b.c"
#endif
#ifndef INC_write_c_B
#include "../../ZUDIR/write_c.b.c"
#endif
CSymbol *MLibIO__FgetSymbol() {
  CSymbol *Vsym;
  Vsym = CSymbol__FNEW(20);
  Vsym->Vname = "IO";
  Vsym->VcName = "MIO";
  ZListAdd(CWrite_C__X__VincludeWriters, -1, 0, MLibIO__FwriteInclude, 1);
  ZListAdd(CWrite_C__X__VtypedefWriters, -1, 0, MLibIO__FwriteTypedef, 1);
  ZListAdd(CWrite_C__X__VdeclWriters, -1, 0, MLibIO__FwriteDecl, 1);
  ZListAdd(CWrite_C__X__VbodyWriters, -1, 0, MLibIO__FwriteBody, 1);
  CSymbol *Vmember;
  Vmember = NULL;
  Vmember = CSymbol__FaddLibMethod(Vsym, "readByte", MLibIO__FreadByte, CSymbol__X__Vint);
  Vmember = CSymbol__FaddLibMethod(Vsym, "readFile", MLibIO__FreadFile, CSymbol__X__Vstring);
  CSymbol__FaddMember(Vmember, "fileName", CSymbol__X__Vstring, 0);
  Vmember = CSymbol__FaddLibMethod(Vsym, "write", MLibIO__Fwrite, CSymbol__X__Vint);
  CSymbol__FaddMember(Vmember, "text", CSymbol__X__Vstring, 0);
  Vmember = CSymbol__FaddLibMethod(Vsym, "writeLine", MLibIO__FwriteLine, CSymbol__X__Vint);
  CSymbol__FaddMember(Vmember, "text", CSymbol__X__Vstring, 0);
  Vmember = CSymbol__FaddLibMethod(Vsym, "writeByte", MLibIO__FwriteByte, CSymbol__X__Vint);
  CSymbol__FaddMember(Vmember, "byte", CSymbol__X__Vint, 0);
  Vmember = CSymbol__FaddLibMethod(Vsym, "flush", MLibIO__Fflush, CSymbol__X__Vint);
  MLibIO__VfileSym = CSymbol__FNEW(21);
  MLibIO__VfileSym->Vname = "File";
  MLibIO__VfileSym->Vclass = MLibIO__VfileSym;
  MLibIO__VfileSym->VcName = "FILE";
  MLibIO__VfileSym->VclassName = "FILE";
  CSymbol__FaddMember__1(Vsym, MLibIO__VfileSym);
  Vmember = CSymbol__FaddLibMethod(MLibIO__VfileSym, "readByte", MLibIO__FfileReadByte, CSymbol__X__Vint);
  Vmember = CSymbol__FaddLibMethod(MLibIO__VfileSym, "write", MLibIO__FfileWrite, NULL);
  CSymbol__FaddMember(Vmember, "text", CSymbol__X__Vstring, 0);
  Vmember = CSymbol__FaddLibMethod(MLibIO__VfileSym, "writeByte", MLibIO__FfileWriteByte, NULL);
  CSymbol__FaddMember(Vmember, "byte", CSymbol__X__Vint, 0);
  Vmember = CSymbol__FaddLibMethod(MLibIO__VfileSym, "close", MLibIO__FfileClose, NULL);
  MLibIO__VfileSym = CSymbol__FcopyObject(MLibIO__VfileSym);
  Vmember = CSymbol__FaddLibMethod(Vsym, "fileReader", MLibIO__FfileReader, MLibIO__VfileSym);
  CSymbol__FaddMember(Vmember, "fileName", CSymbol__X__Vstring, 0);
  Vmember = CSymbol__FaddLibMethod(Vsym, "fileWriter", MLibIO__FfileWriter, MLibIO__VfileSym);
  CSymbol__FaddMember(Vmember, "fileName", CSymbol__X__Vstring, 0);
  CSymbol__FaddMember(Vsym, "eof", CSymbol__FNEW(7), 0)->VcName = "MIO__Veof";
  Vmember = CSymbol__FaddLibMethod(Vsym, "rename", MLibIO__Frename, CSymbol__X__Vint);
  CSymbol__FaddMember(Vmember, "fromName", CSymbol__X__Vstring, 0);
  CSymbol__FaddMember(Vmember, "toName", CSymbol__X__Vstring, 0);
  Vmember = CSymbol__FaddLibMethod(Vsym, "delete", MLibIO__Fdelete, CSymbol__X__Vint);
  CSymbol__FaddMember(Vmember, "fileName", CSymbol__X__Vstring, 0);
  Vmember = CSymbol__FaddLibMethod(Vsym, "mkdir", MLibIO__Fmkdir, CSymbol__X__Vint);
  CSymbol__FaddMember(Vmember, "dirName", CSymbol__X__Vstring, 0);
  CSymbol *VstatSym;
  VstatSym = CSymbol__FNEW(21);
  VstatSym->Vname = "Stat";
  VstatSym->Vclass = VstatSym;
  VstatSym->VcName = "MIO__CStat";
  VstatSym->VclassName = "MIO__CStat";
  CSymbol__FaddMember(VstatSym, "size", CSymbol__X__Vint, 0)->VcName = "size";
  CSymbol__FaddMember(VstatSym, "time", CSymbol__X__Vint, 0)->VcName = "time";
  CSymbol__FaddMember__1(Vsym, VstatSym);
  VstatSym = CSymbol__FcopyObject(VstatSym);
  Vmember = CSymbol__FaddLibMethod(Vsym, "stat", MLibIO__Fstat, VstatSym);
  CSymbol__FaddMember(Vmember, "fileName", CSymbol__X__Vstring, 0);
  return Vsym;
}
void MLibIO__FreadByte(CSymbol *Asym, CSymbol *Aclass, CNode *Aarg_node, CSContext *Actx) {
  COutput__Fwrite(Actx->Vout, "fgetc(stdin)");
}
void MLibIO__FreadFile(CSymbol *Asym, CSymbol *Aclass, CNode *Aarg_node, CSContext *Actx) {
  CScope__FaddUsedItem(Actx->Vscope, "IO.readFile");
  CScope__FaddUsedItem(Actx->Vscope, "fcntl.h");
  CScope__FaddUsedItem(Actx->Vscope, "sys/types.h");
  CScope__FaddUsedItem(Actx->Vscope, "unistd.h");
  COutput__Fwrite(Actx->Vout, "ZreadFile(");
  MLibIO__FgenExpr(Aarg_node, Actx, CSymbol__X__Vstring);
  COutput__Fwrite(Actx->Vout, ")");
}
void MLibIO__Fwrite(CSymbol *Asym, CSymbol *Aclass, CNode *Aarg_node, CSContext *Actx) {
  COutput__Fwrite(Actx->Vout, "fputs(");
  MLibIO__FgenExpr(Aarg_node, Actx, CSymbol__X__Vstring);
  COutput__Fwrite(Actx->Vout, ", stdout)");
}
void MLibIO__FwriteLine(CSymbol *Asym, CSymbol *Aclass, CNode *Aarg_node, CSContext *Actx) {
  COutput__Fwrite(Actx->Vout, "(fputs(");
  MLibIO__FgenExpr(Aarg_node, Actx, CSymbol__X__Vstring);
  COutput__Fwrite(Actx->Vout, ", stdout) | fputc('\\n', stdout))");
}
void MLibIO__FwriteByte(CSymbol *Asym, CSymbol *Aclass, CNode *Aarg_node, CSContext *Actx) {
  COutput__Fwrite(Actx->Vout, "fputc(");
  MLibIO__FgenExpr(Aarg_node, Actx, CSymbol__X__Vint);
  COutput__Fwrite(Actx->Vout, ", stdout)");
}
void MLibIO__Fflush(CSymbol *Asym, CSymbol *Aclass, CNode *Aarg_node, CSContext *Actx) {
  COutput__Fwrite(Actx->Vout, "fflush(stdout)");
}
void MLibIO__FfileReadByte(CSymbol *Asym, CSymbol *Aclass, CNode *Am_node, CSContext *Actx) {
  COutput__Fwrite(Actx->Vout, "fgetc(");
  MLibIO__FgenerateVarname(Am_node->Vn_left->Vn_left, Actx, MLibIO__VfileSym);
  COutput__Fwrite(Actx->Vout, ")");
  Am_node->Vn_undefined = Am_node->Vn_left->Vn_left->Vn_undefined;
}
void MLibIO__FfileWrite(CSymbol *Asym, CSymbol *Aclass, CNode *Am_node, CSContext *Actx) {
  COutput__Fwrite(Actx->Vout, "fputs(");
  MLibIO__FgenExpr(Am_node->Vn_right, Actx, CSymbol__X__Vstring);
  COutput__Fwrite(Actx->Vout, ", ");
  MLibIO__FgenerateVarname(Am_node->Vn_left->Vn_left, Actx, MLibIO__VfileSym);
  COutput__Fwrite(Actx->Vout, ")");
  Am_node->Vn_undefined = (Am_node->Vn_right->Vn_undefined + Am_node->Vn_left->Vn_left->Vn_undefined);
}
void MLibIO__FfileWriteByte(CSymbol *Asym, CSymbol *Aclass, CNode *Am_node, CSContext *Actx) {
  COutput__Fwrite(Actx->Vout, "fputc(");
  MLibIO__FgenExpr(Am_node->Vn_right, Actx, CSymbol__X__Vint);
  COutput__Fwrite(Actx->Vout, ", ");
  MLibIO__FgenerateVarname(Am_node->Vn_left->Vn_left, Actx, MLibIO__VfileSym);
  COutput__Fwrite(Actx->Vout, ")");
  Am_node->Vn_undefined = (Am_node->Vn_right->Vn_undefined + Am_node->Vn_left->Vn_left->Vn_undefined);
}
void MLibIO__FfileClose(CSymbol *Asym, CSymbol *Aclass, CNode *Am_node, CSContext *Actx) {
  COutput__Fwrite(Actx->Vout, "fclose(");
  MLibIO__FgenerateVarname(Am_node->Vn_left->Vn_left, Actx, MLibIO__VfileSym);
  COutput__Fwrite(Actx->Vout, ")");
  Am_node->Vn_undefined = Am_node->Vn_left->Vn_left->Vn_undefined;
}
void MLibIO__FfileReader(CSymbol *Asym, CSymbol *Aclass, CNode *Aarg_node, CSContext *Actx) {
  COutput__Fwrite(Actx->Vout, "fopen(");
  MLibIO__FgenExpr(Aarg_node, Actx, CSymbol__X__Vstring);
  COutput__Fwrite(Actx->Vout, ", \"r\")");
}
void MLibIO__FfileWriter(CSymbol *Asym, CSymbol *Aclass, CNode *Aarg_node, CSContext *Actx) {
  COutput__Fwrite(Actx->Vout, "fopen(");
  MLibIO__FgenExpr(Aarg_node, Actx, CSymbol__X__Vstring);
  COutput__Fwrite(Actx->Vout, ", \"w\")");
}
void MLibIO__Frename(CSymbol *Asym, CSymbol *Aclass, CNode *Aarg_node, CSContext *Actx) {
  COutput__Fwrite(Actx->Vout, "rename(");
  MLibIO__FgenExpr(Aarg_node->Vn_left, Actx, CSymbol__X__Vstring);
  COutput__Fwrite(Actx->Vout, ", ");
  MLibIO__FgenExpr(Aarg_node->Vn_right, Actx, CSymbol__X__Vstring);
  COutput__Fwrite(Actx->Vout, ")");
}
void MLibIO__Fdelete(CSymbol *Asym, CSymbol *Aclass, CNode *Aarg_node, CSContext *Actx) {
  CScope__FaddUsedItem(Actx->Vscope, "unistd.h");
  COutput__Fwrite(Actx->Vout, "unlink(");
  MLibIO__FgenExpr(Aarg_node, Actx, CSymbol__X__Vstring);
  COutput__Fwrite(Actx->Vout, ")");
}
void MLibIO__Fmkdir(CSymbol *Asym, CSymbol *Aclass, CNode *Aarg_node, CSContext *Actx) {
  CScope__FaddUsedItem(Actx->Vscope, "sys/types.h");
  CScope__FaddUsedItem(Actx->Vscope, "sys/stat.h");
  COutput__Fwrite(Actx->Vout, "\n#if defined(__MINGW32__) || defined(_MSC_VER)\n");
  COutput__Fwrite(Actx->Vout, "_mkdir(");
  MLibIO__FgenExpr(Aarg_node, Actx, CSymbol__X__Vstring);
  COutput__Fwrite(Actx->Vout, ")\n");
  COutput__Fwrite(Actx->Vout, "#else\n");
  COutput__Fwrite(Actx->Vout, "mkdir(");
  MLibIO__FgenExpr(Aarg_node, Actx, CSymbol__X__Vstring);
  COutput__Fwrite(Actx->Vout, ", 0777)\n");
  COutput__Fwrite(Actx->Vout, "#endif\n");
}
void MLibIO__Fstat(CSymbol *Asym, CSymbol *Aclass, CNode *Aarg_node, CSContext *Actx) {
  CScope__FaddUsedItem(Actx->Vscope, "sys/stat.h");
  CScope__FaddUsedItem(Actx->Vscope, "alloc");
  MLibIO__VuseZStat = 1;
  COutput__Fwrite(Actx->Vout, "ZStat(");
  MLibIO__FgenExpr(Aarg_node, Actx, CSymbol__X__Vstring);
  COutput__Fwrite(Actx->Vout, ")");
}
CSymbol *MLibIO__FgenerateVarname(CNode *Anode, CSContext *Actx, CSymbol *AdestSym) {
  return MGenerate__FgenerateVarname(Anode, Actx, AdestSym);
}
CSymbol *MLibIO__FgenExpr(CNode *Anode, CSContext *Actx, CSymbol *AdestSym) {
  return MGenerate__FgenExpr(Anode, Actx, AdestSym);
}
void MLibIO__FwriteInclude(CScope *AtopScope, FILE *Afd) {
}
void MLibIO__FwriteTypedef(CScope *AtopScope, FILE *Afd) {
  if (MLibIO__VuseZStat)
  {
    fputs("\ntypedef struct MIO__CStat__S MIO__CStat;\n", Afd);
  }
}
void MLibIO__FwriteDecl(CScope *AtopScope, FILE *Afd) {
  if (MLibIO__VuseZStat)
  {
    fputs("\nstruct MIO__CStat__S {\n  size_t size;\n  long   time;\n};\n", Afd);
  }
}
void MLibIO__FwriteBody(CScope *AtopScope, FILE *Afd) {
  if (MLibIO__VuseZStat)
  {
    fputs("\nMIO__CStat *ZStat(char *name) {\n  MIO__CStat *res = Zalloc(sizeof(MIO__CStat));\n  struct stat st;\n  if (stat(name, &st) == 0) {\n    res->size = st.st_size;\n    res->time = st.st_mtime;\n  }\n  return res;\n}\n", Afd);
  }
  if (ZDictHas(AtopScope->VusedItems, 0, "IO.readFile"))
  {
    fputs("\nchar *ZreadFile(char *fileName) {\n  off_t len;\n  char *res;\n  int fd = open(fileName, O_RDONLY, 0);\n  if (fd < 0) {\n    perror(\"readFile open\");\n    return \"\";\n  }\n  len = lseek(fd, 0, SEEK_END);\n  if (len < 0) {\n    perror(\"readFile lseek\");\n    close(fd);\n    return \"\";\n  }\n  if (lseek(fd, 0, SEEK_SET) < 0) {\n    perror(\"readFile reset\");\n    close(fd);\n    return \"\";\n  }\n  res = malloc(len + 1);\n  if (read(fd, res, len) != len) {\n    perror(\"readFile read\");\n    close(fd);\n    return \"\";\n  }\n  res[len] = 0;\n  close(fd);\n  return res;\n}\n", Afd);
  }
}
void Ilibio() {
 static int done = 0;
 if (!done) {
  done = 1;
  Igenerate();
  Ioutput();
  Iscontext();
  Iscope();
  Isymbol();
  Iwrite_c();
  MLibIO__VfileSym = NULL;
  MLibIO__VuseZStat = 0;
 }
}
