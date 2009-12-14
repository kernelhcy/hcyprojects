#define INC_zwtmodule_B 1
/*
 * FUNCTION BODIES
 */
void MZWTmodule__FcreatePage(MHTTPmodule__CServer *Aserver, char *ApageName, MINFOmodule__CModuleInfo *Ainfo) {
  char *Vleader;
  Vleader = "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01 Transitional//EN\">\n<html>\n<head>\n  <meta http-equiv=\"Content-Type\" content=\"text/html; charset=UTF-8\">\n  <title>";
  char *Vmiddle;
  Vmiddle = "</title>\n  <script type=\"text/javascript\">\n    var $wnd = parent;\n    var $doc = $wnd.document;\n  </script>\n</head>\n<body>\n  <iframe src=\"javascript:''\" id=\"__zwt_historyFrame\" tabIndex='-1' style=\"position:absolute;width:0;height:0;border:0\"></iframe>\n  <script type=\"text/javascript\">\n  function jsType() {\n    var agent = navigator.userAgent.toLowerCase();\n    var makeVersion = function(vers) {\n      return parseInt(vers[1]) * 1000 + parseInt(vers[2]);\n    };\n    if (agent.indexOf('opera') != -1) { return 'opera'; }\n    else if (agent.indexOf('webkit') != -1) { return 'safari'; }\n    else if (agent.indexOf('msie') != -1) {\n      if (document.documentMode >= 8) { return 'ie8'; }\n      else {\n        var ievers = /msie ([0-9]+)\\.([0-9]+)/.exec(agent);\n        if (ievers && ievers.length == 3) {\n          var v = makeVersion(ievers);\n          if (v >= 6000) { return 'ie6'; }\n        }\n      }\n    }\n    else if (agent.indexOf('gecko') != -1) {\n      var geckovers = /rv:([0-9]+)\\.([0-9]+)/.exec(agent);\n      if (geckovers && geckovers.length == 3) {\n        if (makeVersion(geckovers) >= 1008)\n          return 'gecko18';\n      }\n      return 'gecko';\n    }\n    return 'unknown';\n  };\n  function getPageName() {\n    var fnames = {";
  char *Vfooter;
  Vfooter = "};\n    var name = jsType();\n    return fnames[name];\n  };\n  function loadIframe(fname) {\n    var iframe = $doc.createElement('iframe');\n    iframe.src = \"javascript:''\";\n    iframe.id = 'demoapp';\n    iframe.style.cssText = 'position:absolute;width:0;height:0;border:none';\n    iframe.tabIndex = -1;\n    $doc.body.appendChild(iframe);\n    iframe.contentWindow.location.replace(fname);\n  };\n  var pname = getPageName();\n  if (pname && pname != 'unknown') {\n    loadIframe(pname);\n  } else {\n    var e = $doc.createElement('div');\n    e.innerHTML = \"Sorry, your browser is not supported.\";\n    $doc.body.appendChild(e);\n  }\n  </script>\n</body>\n</html>\n";
  char *Vhtml;
  Vhtml = Zconcat(Zconcat(Vleader, Ainfo->Vname), Vmiddle);
  {
    Zfor_T *Zf = ZforNew(ZDictKeys(Ainfo->Vpermutations), 2);
    char *Vkey;
    for (ZforGetPtr(Zf, (char **)&Vkey); ZforCont(Zf); ZforNextPtr(Zf, (char **)&Vkey)) {
      char *VjsName;
      VjsName = ZDictGetPtr(Ainfo->Vpermutations, 0, Vkey);
      Vhtml = Zconcat(Vhtml, Zconcat(Zconcat(Zconcat(Zconcat("'", Vkey), "': '/"), VjsName), "', "));
      char *Vpath;
      Vpath = "";
      MHTTPmodule__CFileServlet *Vfs;
      Vfs = MHTTPmodule__CFileServlet__FNEW(Zconcat(Vpath, VjsName));
      MHTTPmodule__CFileServlet__FaddPath__1(Vfs, Zconcat("/", VjsName));
      MHTTPmodule__CServer__FaddServlet(Aserver, ZallocZoref(Vfs, 0));
    }
  }
  Vhtml = Zconcat(Vhtml, "'unknown': 'unknown'");
  Vhtml = Zconcat(Vhtml, Vfooter);
  MHTTPmodule__CStringServlet *Vss;
  Vss = MHTTPmodule__CStringServlet__FNEW(Vhtml);
  MHTTPmodule__CStringServlet__FaddPath__1(Vss, Zconcat("/", ApageName));
  MHTTPmodule__CServer__FaddServlet(Aserver, ZallocZoref(Vss, 1));
}
