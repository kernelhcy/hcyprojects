#define INC_zwtDemoPage_B 1
/*
 * FUNCTION BODIES
 */
#ifndef INC_zwtmodule_B
#include ".././lib/ZUDIR/zwtmodule.b.c"
#endif
MINFOmodule__CModuleInfo *MZwtDemoPage__FINFO() {
  MINFOmodule__CModuleInfo *info = (MINFOmodule__CModuleInfo *)Zalloc(sizeof(MINFOmodule__CModuleInfo));
  info->Vname = "ZwtDemoPage";
  info->Vpermutations = ZnewDict(1);
  ZDictAdd(0, info->Vpermutations, 0, "ie6", 0, "ZwtDemoPage.10105.html");
  ZDictAdd(0, info->Vpermutations, 0, "ie8", 0, "ZwtDemoPage.10105.html");
  ZDictAdd(0, info->Vpermutations, 0, "opera", 0, "ZwtDemoPage.10105.html");
  ZDictAdd(0, info->Vpermutations, 0, "gecko", 0, "ZwtDemoPage.10107.html");
  ZDictAdd(0, info->Vpermutations, 0, "safari", 0, "ZwtDemoPage.10105.html");
  ZDictAdd(0, info->Vpermutations, 0, "gecko18", 0, "ZwtDemoPage.10107.html");
  return info;
}
