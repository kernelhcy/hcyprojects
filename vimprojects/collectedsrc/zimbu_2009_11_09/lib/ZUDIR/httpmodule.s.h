#define INC_httpmodule_S 1
/*
 * STRUCTS
 */
char *MHTTPmodule__ERequestType[] = {
"options",
"get",
"head",
"post",
"put",
"delete",
"trace",
"connect",
};
struct MHTTPmodule__CRequest__S {
  Zenum Vtype;
  CDictHead *VheaderItems;
  char *Vpath;
  char *Vparams;
};
struct MHTTPmodule__CResponse__S {
  Zint Vcode;
  char *Vmsg;
  CDictHead *VheaderItems;
  char *Vbody;
};
struct MHTTPmodule__CFileServlet__S {
  char *Vname;
  CListHead *Vpaths;
  char *VmimeType;
  char *VfileName;
};
struct MHTTPmodule__CStringServlet__S {
  char *Vname;
  CListHead *Vpaths;
  char *VmimeType;
  char *Vbody;
};
struct MHTTPmodule__CFunctionServlet__S {
  char *Vname;
  CListHead *Vpaths;
  char *VmimeType;
  void *VserveFunc;
};
struct MHTTPmodule__CServer__S {
  char *name;
  int state;
  int type;
  int keep;
  void (*proc)();
  pthread_t id;
  Zint Vport;
  Zint VlistenQueueLen;
  CListHead *Vservlets;
  char *VfileRoot;
};
