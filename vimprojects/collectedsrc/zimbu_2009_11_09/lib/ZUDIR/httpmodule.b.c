#define INC_httpmodule_B 1
/*
 * FUNCTION BODIES
 */
MHTTPmodule__CResponse *MHTTPmodule__CResponse__FNEW() {
  MHTTPmodule__CResponse *THIS = Zalloc(sizeof(MHTTPmodule__CResponse));
  THIS->Vcode = 200;
  THIS->Vmsg = "OK";
  THIS->VheaderItems = ZnewDict(1);
  MHTTPmodule__CResponse__FsetType(THIS, "text/html; charset=utf-8");
  THIS->Vbody = "empty\n";
  return THIS;
}
void MHTTPmodule__CResponse__FsetLength(MHTTPmodule__CResponse *THIS, Zint Alength) {
  ZDictAdd(1, THIS->VheaderItems, 0, "Content-Length", 0, Zint2string(Alength));
}
void MHTTPmodule__CResponse__FsetType(MHTTPmodule__CResponse *THIS, char *Atype) {
  ZDictAdd(1, THIS->VheaderItems, 0, "Content-Type", 0, Atype);
}
MHTTPmodule__CFileServlet *MHTTPmodule__CFileServlet__FNEW(char *A_fileName) {
  MHTTPmodule__CFileServlet *THIS = Zalloc(sizeof(MHTTPmodule__CFileServlet));
  THIS->Vname = "FileServlet";
  THIS->VfileName = A_fileName;
  return THIS;
}
void MHTTPmodule__CFileServlet__FsetFile(MHTTPmodule__CFileServlet *THIS, char *A_fileName) {
  THIS->VfileName = A_fileName;
}
void MHTTPmodule__CFileServlet__Fserve__1(MHTTPmodule__CFileServlet *THIS, MHTTPmodule__CServer *Aserver, MHTTPmodule__CRequest *Areq, MHTTPmodule__CResponse *Aresp) {
  (fputs(Zconcat(Zconcat(Zconcat("reading ", Aserver->VfileRoot), "/"), THIS->VfileName), stdout) | fputc('\n', stdout));
  Aresp->Vbody = ZreadFile(Zconcat(Zconcat(Aserver->VfileRoot, "/"), THIS->VfileName));
}
void MHTTPmodule__CFileServlet__FsetMimeType__1(MHTTPmodule__CFileServlet *THIS, char *Atype) {
  THIS->VmimeType = Atype;
}
Zbool MHTTPmodule__CFileServlet__Fhandle__1(MHTTPmodule__CFileServlet *THIS, MHTTPmodule__CServer *Aserver, MHTTPmodule__CRequest *Areq, MHTTPmodule__CResponse *Aresp) {
  if ((THIS->Vpaths != NULL))
  {
    {
      Zfor_T *Zf = ZforNew(THIS->Vpaths, 2);
      char *Vp;
      for (ZforGetPtr(Zf, (char **)&Vp); ZforCont(Zf); ZforNextPtr(Zf, (char **)&Vp)) {
        if ((Zstrcmp(Vp, Areq->Vpath) == 0))
        {
          if ((THIS->VmimeType != NULL))
          {
            MHTTPmodule__CResponse__FsetType(Aresp, THIS->VmimeType);
          }
          MHTTPmodule__CFileServlet__Fserve__1(THIS, Aserver, Areq, Aresp);
          return 1;
        }
      }
    }
  }
  return 0;
}
void MHTTPmodule__CFileServlet__FaddPath__1(MHTTPmodule__CFileServlet *THIS, char *Apath) {
  if ((THIS->Vpaths == NULL))
  {
    THIS->Vpaths = Zalloc(sizeof(CListHead));
  }
  ZListAdd(THIS->Vpaths, -1, 0, Apath, 1);
}
MHTTPmodule__CStringServlet *MHTTPmodule__CStringServlet__FNEW(char *A_body) {
  MHTTPmodule__CStringServlet *THIS = Zalloc(sizeof(MHTTPmodule__CStringServlet));
  THIS->Vname = "StringServlet";
  THIS->Vbody = A_body;
  return THIS;
}
void MHTTPmodule__CStringServlet__FsetBody(MHTTPmodule__CStringServlet *THIS, char *A_body) {
  THIS->Vbody = A_body;
}
void MHTTPmodule__CStringServlet__Fserve__1(MHTTPmodule__CStringServlet *THIS, MHTTPmodule__CServer *Aserver, MHTTPmodule__CRequest *Areq, MHTTPmodule__CResponse *Aresp) {
  Aresp->Vbody = THIS->Vbody;
}
void MHTTPmodule__CStringServlet__FsetMimeType__1(MHTTPmodule__CStringServlet *THIS, char *Atype) {
  THIS->VmimeType = Atype;
}
Zbool MHTTPmodule__CStringServlet__Fhandle__1(MHTTPmodule__CStringServlet *THIS, MHTTPmodule__CServer *Aserver, MHTTPmodule__CRequest *Areq, MHTTPmodule__CResponse *Aresp) {
  if ((THIS->Vpaths != NULL))
  {
    {
      Zfor_T *Zf = ZforNew(THIS->Vpaths, 2);
      char *Vp;
      for (ZforGetPtr(Zf, (char **)&Vp); ZforCont(Zf); ZforNextPtr(Zf, (char **)&Vp)) {
        if ((Zstrcmp(Vp, Areq->Vpath) == 0))
        {
          if ((THIS->VmimeType != NULL))
          {
            MHTTPmodule__CResponse__FsetType(Aresp, THIS->VmimeType);
          }
          MHTTPmodule__CStringServlet__Fserve__1(THIS, Aserver, Areq, Aresp);
          return 1;
        }
      }
    }
  }
  return 0;
}
void MHTTPmodule__CStringServlet__FaddPath__1(MHTTPmodule__CStringServlet *THIS, char *Apath) {
  if ((THIS->Vpaths == NULL))
  {
    THIS->Vpaths = Zalloc(sizeof(CListHead));
  }
  ZListAdd(THIS->Vpaths, -1, 0, Apath, 1);
}
MHTTPmodule__CFunctionServlet *MHTTPmodule__CFunctionServlet__FNEW(void *Afunc) {
  MHTTPmodule__CFunctionServlet *THIS = Zalloc(sizeof(MHTTPmodule__CFunctionServlet));
  THIS->Vname = "FunctionServlet";
  THIS->VserveFunc = Afunc;
  return THIS;
}
void MHTTPmodule__CFunctionServlet__FsetProc(MHTTPmodule__CFunctionServlet *THIS, void *Afunc) {
  THIS->VserveFunc = Afunc;
}
void MHTTPmodule__CFunctionServlet__Fserve__1(MHTTPmodule__CFunctionServlet *THIS, MHTTPmodule__CServer *Aserver, MHTTPmodule__CRequest *Areq, MHTTPmodule__CResponse *Aresp) {
  Aresp->Vbody = ((char * (*)(MHTTPmodule__CRequest *, MHTTPmodule__CResponse *))THIS->VserveFunc)(Areq, Aresp);
}
void MHTTPmodule__CFunctionServlet__FsetMimeType__1(MHTTPmodule__CFunctionServlet *THIS, char *Atype) {
  THIS->VmimeType = Atype;
}
Zbool MHTTPmodule__CFunctionServlet__Fhandle__1(MHTTPmodule__CFunctionServlet *THIS, MHTTPmodule__CServer *Aserver, MHTTPmodule__CRequest *Areq, MHTTPmodule__CResponse *Aresp) {
  if ((THIS->Vpaths != NULL))
  {
    {
      Zfor_T *Zf = ZforNew(THIS->Vpaths, 2);
      char *Vp;
      for (ZforGetPtr(Zf, (char **)&Vp); ZforCont(Zf); ZforNextPtr(Zf, (char **)&Vp)) {
        if ((Zstrcmp(Vp, Areq->Vpath) == 0))
        {
          if ((THIS->VmimeType != NULL))
          {
            MHTTPmodule__CResponse__FsetType(Aresp, THIS->VmimeType);
          }
          MHTTPmodule__CFunctionServlet__Fserve__1(THIS, Aserver, Areq, Aresp);
          return 1;
        }
      }
    }
  }
  return 0;
}
void MHTTPmodule__CFunctionServlet__FaddPath__1(MHTTPmodule__CFunctionServlet *THIS, char *Apath) {
  if ((THIS->Vpaths == NULL))
  {
    THIS->Vpaths = Zalloc(sizeof(CListHead));
  }
  ZListAdd(THIS->Vpaths, -1, 0, Apath, 1);
}
MHTTPmodule__CServer *MHTTPmodule__CServer__FNEW__1(Zint A_port) {
  MHTTPmodule__CServer *THIS = Zalloc(sizeof(MHTTPmodule__CServer));
  THIS->Vport = A_port;
  THIS->Vservlets = Zalloc(sizeof(CListHead));
  return THIS;
}
void MHTTPmodule__CServer__FaddServlet(MHTTPmodule__CServer *THIS, Zoref *Aservlet) {
  ZListAdd(THIS->Vservlets, -1, 0, Aservlet, 1);
}
void MHTTPmodule__CServer__Fbody__1(MHTTPmodule__CServer *THIS) {
  char *VreqString;
  VreqString = NULL;
  Zint VlistenArg;
  VlistenArg = THIS->VlistenQueueLen;
      int serv_fd;
      serv_fd = socket(AF_INET, SOCK_STREAM, 0);
      if (serv_fd == -1) {
        perror("HTTP server socket");
        return;
      }
      {
        int on = 1;
        if (setsockopt(serv_fd, SOL_SOCKET, SO_REUSEADDR,
                                                     &on, sizeof(on)) == -1) {
          perror("HTTP server setsockopt");
          return;
        }
      }
      {
        struct sockaddr_in serv_addr;
        memset(&serv_addr, 0, sizeof(struct sockaddr_in));
        serv_addr.sin_family = AF_INET;
        serv_addr.sin_addr.s_addr = INADDR_ANY;
        serv_addr.sin_port = htons((int)THIS->Vport);
        if (bind(serv_fd, (struct sockaddr *)&serv_addr,
                                                   sizeof(serv_addr)) == -1) {
          perror("HTTP server bind");
          return;
        }
      }

      if (listen(serv_fd, VlistenArg) == -1) {
        perror("HTTP server listen");
        return;
      }
      while (1) {
        struct sockaddr_in client_addr;
        int fd;
  #define MAX_HEADER_SIZE 4100
        char req_string[MAX_HEADER_SIZE];
        socklen_t addr_len = sizeof(client_addr);
        int len;

        fd = accept(serv_fd, (struct sockaddr *)&client_addr, &addr_len);
        if (fd == -1) {
          perror("HTTP server accept");
          return;
        }
        len = read(fd, req_string, MAX_HEADER_SIZE - 1);
        if (len < 0) {
          perror("HTTP server read");
          return;
        }
        req_string[len] = 0;
        VreqString = req_string;
  MHTTPmodule__CRequest *Vreq;
  Vreq = Zalloc(sizeof(MHTTPmodule__CRequest));
  Zint Vsi;
  Vsi = ZStringIndex(VreqString, 32);
  Vreq->Vtype = 1;
  while ((VreqString[Vsi] == 32))
  {
    ++(Vsi);
  }
  Zint Vei;
  Vei = Vsi;
  while (((VreqString[Vei] != 32) && (VreqString[Vei] != 0)))
  {
    ++(Vei);
  }
  Vreq->Vpath = ZStringSlice(VreqString, Vsi, (Vei - 1));
  Zint Vqi;
  Vqi = ZStringIndex(Vreq->Vpath, 63);
  if ((Vqi >= 0))
  {
    Vreq->Vparams = ZStringSlice(Vreq->Vpath, (Vqi + 1), -(1));
    Vreq->Vpath = ZStringSlice(Vreq->Vpath, 0, (Vqi - 1));
  }
  (fputs(Zconcat("Received HTTP request ", ZStringSlice(VreqString, 0, (Vei - 1))), stdout) | fputc('\n', stdout));
  MHTTPmodule__CResponse *Vresp;
  Vresp = MHTTPmodule__CResponse__FNEW();
  Zbool Vdone;
  Vdone = 0;
  {
    Zfor_T *Zf = ZforNew(THIS->Vservlets, 2);
    Zoref *Vservlet;
    for (ZforGetPtr(Zf, (char **)&Vservlet); ZforCont(Zf); ZforNextPtr(Zf, (char **)&Vservlet)) {
      if (MHTTPmodule__CServlet_I__Mhandle__MHTTPmodule__CServer__MHTTPmodule__CRequest__MHTTPmodule__CResponse_ptr[Vservlet->type](Vservlet->ptr, THIS, Vreq, Vresp))
      {
        Vdone = 1;
        break;
      }
    }
  }
  if (!(Vdone))
  {
    Vresp->Vcode = 404;
    Vresp->Vmsg = "Page not found.";
    Vresp->Vbody = "Sorry, this page is not available.\n";
  }
  MHTTPmodule__CResponse__FsetLength(Vresp, strlen(Vresp->Vbody));
  char *VrespString;
  VrespString = Zconcat(Zconcat(Zconcat(Zconcat("HTTP/1.1 ", Zint2string(Vresp->Vcode)), " "), Vresp->Vmsg), "\r\n");
  Zint VrespLen;
  VrespLen = 0;
  {
    Zfor_T *Zf = ZforNew(ZDictKeys(Vresp->VheaderItems), 2);
    char *Vkey;
    for (ZforGetPtr(Zf, (char **)&Vkey); ZforCont(Zf); ZforNextPtr(Zf, (char **)&Vkey)) {
      VrespString = Zconcat(VrespString, Zconcat(Zconcat(Zconcat(Vkey, ": "), ZDictGetPtr(Vresp->VheaderItems, 0, Vkey)), "\r\n"));
    }
  }
  VrespLen = strlen(VrespString);
  (fputs(Zconcat("Sending HTTP response ", Zint2string(Vresp->Vcode)), stdout) | fputc('\n', stdout));
        if (write(fd, VrespString, (size_t)VrespLen) <= 0) {
          perror("write header");
          return;
        }
  if ((Vreq->Vtype != 2))
  {
    VrespString = Vresp->Vbody;
    VrespLen = strlen(VrespString);
          if (write(fd, "\r\n", 2) != 2) {
            perror("write separator");
            return;
          }
          if (write(fd, VrespString, (size_t)VrespLen) <= 0) {
            perror("write body");
            return;
          }
  }
        close(fd);
     }  /* while */
}
