#define INC_httpmodule_D 1
/*
 * DECLARE FUNCTIONS AND GLOBALS
 */
MHTTPmodule__CResponse *MHTTPmodule__CResponse__FNEW();
void MHTTPmodule__CResponse__FsetLength(MHTTPmodule__CResponse *THIS, Zint Alength);
void MHTTPmodule__CResponse__FsetType(MHTTPmodule__CResponse *THIS, char *Atype);
MHTTPmodule__CFileServlet *MHTTPmodule__CFileServlet__FNEW(char *A_fileName);
void MHTTPmodule__CFileServlet__FsetFile(MHTTPmodule__CFileServlet *THIS, char *A_fileName);
void MHTTPmodule__CFileServlet__Fserve__1(MHTTPmodule__CFileServlet *THIS, MHTTPmodule__CServer *Aserver, MHTTPmodule__CRequest *Areq, MHTTPmodule__CResponse *Aresp);
void MHTTPmodule__CFileServlet__FsetMimeType__1(MHTTPmodule__CFileServlet *THIS, char *Atype);
Zbool MHTTPmodule__CFileServlet__Fhandle__1(MHTTPmodule__CFileServlet *THIS, MHTTPmodule__CServer *Aserver, MHTTPmodule__CRequest *Areq, MHTTPmodule__CResponse *Aresp);
void MHTTPmodule__CFileServlet__FaddPath__1(MHTTPmodule__CFileServlet *THIS, char *Apath);
MHTTPmodule__CStringServlet *MHTTPmodule__CStringServlet__FNEW(char *A_body);
void MHTTPmodule__CStringServlet__FsetBody(MHTTPmodule__CStringServlet *THIS, char *A_body);
void MHTTPmodule__CStringServlet__Fserve__1(MHTTPmodule__CStringServlet *THIS, MHTTPmodule__CServer *Aserver, MHTTPmodule__CRequest *Areq, MHTTPmodule__CResponse *Aresp);
void MHTTPmodule__CStringServlet__FsetMimeType__1(MHTTPmodule__CStringServlet *THIS, char *Atype);
Zbool MHTTPmodule__CStringServlet__Fhandle__1(MHTTPmodule__CStringServlet *THIS, MHTTPmodule__CServer *Aserver, MHTTPmodule__CRequest *Areq, MHTTPmodule__CResponse *Aresp);
void MHTTPmodule__CStringServlet__FaddPath__1(MHTTPmodule__CStringServlet *THIS, char *Apath);
MHTTPmodule__CFunctionServlet *MHTTPmodule__CFunctionServlet__FNEW(void *Afunc);
void MHTTPmodule__CFunctionServlet__FsetProc(MHTTPmodule__CFunctionServlet *THIS, void *Afunc);
void MHTTPmodule__CFunctionServlet__Fserve__1(MHTTPmodule__CFunctionServlet *THIS, MHTTPmodule__CServer *Aserver, MHTTPmodule__CRequest *Areq, MHTTPmodule__CResponse *Aresp);
void MHTTPmodule__CFunctionServlet__FsetMimeType__1(MHTTPmodule__CFunctionServlet *THIS, char *Atype);
Zbool MHTTPmodule__CFunctionServlet__Fhandle__1(MHTTPmodule__CFunctionServlet *THIS, MHTTPmodule__CServer *Aserver, MHTTPmodule__CRequest *Areq, MHTTPmodule__CResponse *Aresp);
void MHTTPmodule__CFunctionServlet__FaddPath__1(MHTTPmodule__CFunctionServlet *THIS, char *Apath);
MHTTPmodule__CServer *MHTTPmodule__CServer__FNEW__1(Zint A_port);
void MHTTPmodule__CServer__FaddServlet(MHTTPmodule__CServer *THIS, Zoref *Aservlet);
void MHTTPmodule__CServer__Fbody__1(MHTTPmodule__CServer *THIS);
void  (*(MHTTPmodule__CServlet_I__Mserve__MHTTPmodule__CServer__MHTTPmodule__CRequest__MHTTPmodule__CResponse_ptr[]))(void *, MHTTPmodule__CServer *, MHTTPmodule__CRequest *, MHTTPmodule__CResponse *) = {
    (void  (*)(void *, MHTTPmodule__CServer *, MHTTPmodule__CRequest *, MHTTPmodule__CResponse *))MHTTPmodule__CFileServlet__Fserve__1,
  (void  (*)(void *, MHTTPmodule__CServer *, MHTTPmodule__CRequest *, MHTTPmodule__CResponse *))MHTTPmodule__CStringServlet__Fserve__1,
  (void  (*)(void *, MHTTPmodule__CServer *, MHTTPmodule__CRequest *, MHTTPmodule__CResponse *))MHTTPmodule__CFunctionServlet__Fserve__1,
};
void  (*(MHTTPmodule__CServlet_I__MsetMimeType__string_ptr[]))(void *, char *) = {
    (void  (*)(void *, char *))MHTTPmodule__CFileServlet__FsetMimeType__1,
  (void  (*)(void *, char *))MHTTPmodule__CStringServlet__FsetMimeType__1,
  (void  (*)(void *, char *))MHTTPmodule__CFunctionServlet__FsetMimeType__1,
};
Zbool  (*(MHTTPmodule__CServlet_I__Mhandle__MHTTPmodule__CServer__MHTTPmodule__CRequest__MHTTPmodule__CResponse_ptr[]))(void *, MHTTPmodule__CServer *, MHTTPmodule__CRequest *, MHTTPmodule__CResponse *) = {
    (Zbool  (*)(void *, MHTTPmodule__CServer *, MHTTPmodule__CRequest *, MHTTPmodule__CResponse *))MHTTPmodule__CFileServlet__Fhandle__1,
  (Zbool  (*)(void *, MHTTPmodule__CServer *, MHTTPmodule__CRequest *, MHTTPmodule__CResponse *))MHTTPmodule__CStringServlet__Fhandle__1,
  (Zbool  (*)(void *, MHTTPmodule__CServer *, MHTTPmodule__CRequest *, MHTTPmodule__CResponse *))MHTTPmodule__CFunctionServlet__Fhandle__1,
};
void  (*(MHTTPmodule__CServlet_I__MaddPath__string_ptr[]))(void *, char *) = {
    (void  (*)(void *, char *))MHTTPmodule__CFileServlet__FaddPath__1,
  (void  (*)(void *, char *))MHTTPmodule__CStringServlet__FaddPath__1,
  (void  (*)(void *, char *))MHTTPmodule__CFunctionServlet__FaddPath__1,
};
int MHTTPmodule__CServlet_I__VmimeType_off[] = {
  (char *)&((MHTTPmodule__CFileServlet *)&dummy)->VmimeType - (char *)(MHTTPmodule__CFileServlet *)&dummy,
  (char *)&((MHTTPmodule__CStringServlet *)&dummy)->VmimeType - (char *)(MHTTPmodule__CStringServlet *)&dummy,
  (char *)&((MHTTPmodule__CFunctionServlet *)&dummy)->VmimeType - (char *)(MHTTPmodule__CFunctionServlet *)&dummy,
};
int MHTTPmodule__CServlet_I__Vpaths_off[] = {
  (char *)&((MHTTPmodule__CFileServlet *)&dummy)->Vpaths - (char *)(MHTTPmodule__CFileServlet *)&dummy,
  (char *)&((MHTTPmodule__CStringServlet *)&dummy)->Vpaths - (char *)(MHTTPmodule__CStringServlet *)&dummy,
  (char *)&((MHTTPmodule__CFunctionServlet *)&dummy)->Vpaths - (char *)(MHTTPmodule__CFunctionServlet *)&dummy,
};
int MHTTPmodule__CServlet_I__Vname_off[] = {
  (char *)&((MHTTPmodule__CFileServlet *)&dummy)->Vname - (char *)(MHTTPmodule__CFileServlet *)&dummy,
  (char *)&((MHTTPmodule__CStringServlet *)&dummy)->Vname - (char *)(MHTTPmodule__CStringServlet *)&dummy,
  (char *)&((MHTTPmodule__CFunctionServlet *)&dummy)->Vname - (char *)(MHTTPmodule__CFunctionServlet *)&dummy,
};
