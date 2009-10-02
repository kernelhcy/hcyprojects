# -*- coding: UTF-8 -*-

'''
所有输入字符串全部采用unicode编码

client.login(user, pass)
    功能: 登录
    参数:
        user: 用户id
        pass: 密码
    返回值:
        True: 成功
        False: 失败

client.logout()
    功能: 登录
    参数: 无
    返回值:
        True: 成功
        False: 失败

client.is_login()
    功能: 是否处于登录状态
    参数: 无
    返回值:
        True: 登录中
        False: 没有登录或者检测失败

client.post(article)
    功能: 发帖
    参数:
        article: 帖子
        article["title"]: 标题
        article["board"]: 版面
        article["text"]: 正文
    返回值:
        True: 成功
        False: 失败
    NOTE:
        如果要带附件，可以先用upload_one_file()上载附件

client.post_force()
    跟post一样，只不过会把帖子内容自动转义，绕开违禁词汇

client.send_mail_to(mail)
    功能: 发送邮件
    参数:
        mail: 邮件,是个dict
            mail['title']: 标题
            mail['text']: 正文
            mail['userid']: 接受者id
    返回值:
        True: 成功
        False: 失败
    NOTE:
        如果要带附件，可以先用upload_one_file()上载附件

client.upload_one_file(file)
    功能: 上载一个文件到缓冲区
    参数:
        file: 要上载的文件的路径名
    返回值:
        True: 成功
        False: 失败

client.get_board_list()
    功能: 获取版面列表
    参数: 无
    返回值:
        False: 获取失败
        list: [("linux","linux"), ("love","因为有爱"), ...]
'''

import urllib.parse, re, mimetypes
import http.client, os.path, xml.sax.saxutils

class client:
    debug = False
    user = None
    pwd = None
    cookie = None
    conn = None
    host = "bbs.xjtu.edu.cn"
    port = 80
    headers = {"User-Agent": "Mozilla",
                "Keep-alive": "300",
                "Connection": "keep-alive"
                }

    prepend = "Post by libbmy\n" + "" + "="*64 + "\n\n"
    true_or_false = lambda s,x: x!=False and True

    def __del__(self):
        try:
            if self.conn:
                self.conn.close()
        except: pass

    def http_post_get(self, selector, body=None, headers=None):
        def do_request():
            if body==None:
                self.conn.request("GET", selector, headers=headers)
            else:
                self.conn.request("POST", selector, body, headers)
            ret = self.conn.getresponse()
            str = ret.read().decode("gbk")
            return str
        if self.conn==None:
            self.conn = http.client.HTTPConnection(self.host, self.port)
        if headers==None:
            headers = self.headers.copy()
        else:
            L = self.headers.items()
            for k,v in L:
                headers[k] = v
        try:
            str = do_request()
        except:
            try:
                self.conn.close()
                self.conn.connect()
                str = do_request()
            except:
                return False
        return str

    def get_page(self, selector, data=None, headers=None):
        if type and type(data)==type({}):
            L = data.items()
            d = []
            for k,v in L:
                k = urllib.parse.quote_plus(k.encode("gbk"))
                v = urllib.parse.quote_plus(v.encode("gbk"))
                d.append("&"+k+"="+v)
            data = "".join(d)
            data = data[1:]
            print("get_page : data = ",data)
        page = self.http_post_get(selector, data, headers)
        if page:
            page = xml.sax.saxutils.unescape(page)
        print("get_page: page =' ",page, " '")
        return page

    def parse_item(self, selector, data=None, pattern=None, headers=None):
        def get_value(pattern, str):
            if not str:
                return False
            r = re.findall(pattern, str, re.S+re.I)
            if r==None:
                return False
            print("Get_value: r = ",r)
            return r
        page = self.get_page(selector, data, headers)
        if not page:
            return False
        v = get_value(pattern, page)
        if len(v)==0:
            return False
        if len(v)==1:
            v = v[0]
        print("Parse_item : v = ",v)
        return v

    def login(self, user, pwd):
        selector = "/BMY/bbslogin"
        pattern = "(?<=url=/).+?(?=/)"
        data = {"id":user, "pw":pwd}
        v = self.parse_item(selector, data, pattern)
        if v==False:
            return v
        self.cookie = "/" + v + "/"
        self.user = user
        self.pwd = pwd
        return True

    def logout(self):
        selector = self.cookie+"bbslogout"
        pattern = "url=/"
        v = self.parse_item(selector, pattern=pattern)
        v = self.true_or_false(v)
        return v

    def is_login(self):
        if self.cookie==None:
            return False
        selector = self.cookie + "bbsfoot"
        pattern = "(?<=帐号\[<a href=bbsqry\?userid=)(.+?) "
        v = self.parse_item(selector, pattern=pattern)
        if v==False:
            return v
        v = v.lower()==self.user.lower()
        return v

    def post(self, article=None):
        def check(article):
            if not article:
                return False
            if not "board" in article:
                return False
            if not "title" in article:
                return False
            if  not "text" in article:
                article["text"] = ""
            return True
        if not check(article):
            return False
        if self.debug:
            article["text"] = self.prepend + article["text"]
        selector = self.cookie+"bbssnd?board="+article["board"]+"&th=-1"
        pattern = "http-equiv='Refresh'"
        v = self.parse_item(selector, article, pattern)
        v = self.true_or_false(v)
        return v


    def upload_one_file(self, file):
        def encode_file(file):
            boundary = '-----------------------------265001916915724'
            CRLF = '\r\n'
            L = []
            L.append('--'+boundary+CRLF)
            L.append('Content-Disposition: form-data; name="file"; filename="%s"'
                                                 % os.path.basename(file))
            L.append(CRLF)
            L.append('Content-Type: %s' % get_content_type(file))
            L.append(CRLF+CRLF)
            body = "".join(L)
            body = body.encode('gbk')
            f = open(file, "rb")
            body += f.read()
            f.close()
            body += (CRLF+'--'+boundary+"--"+CRLF).encode('gbk')
            content_type = 'multipart/form-data; boundary=%s' % boundary
            return content_type, body

        def get_content_type(file):
            return mimetypes.guess_type(file)[0] or 'application/octet-stream'

        content_type, data = encode_file(file)
        selector = "/cgi-bin/bbs/upload/"+self.cookie[4:-3]
        headers = {"Content-Type": content_type}
        pattern = "(?<=文件 ).+?(?= 上载成功)"
        v = self.parse_item(selector, data, pattern, headers)
        v = self.true_or_false(v)
        return v

    def post_force(self, article):
        article = article.copy()
        if not "text" in article:
            article["text"] = ""
        L = []
        for w in article["text"]:
            L.append(w+"")
        article["text"] = "".join(L)
        return self.post(article)

    def get_board_list(self):
        selector = self.cookie + "bbsall"
        pattern = "(?<=<a href=bbshome\?board=)(.+?)>(.+?)(?=</a>)"
        list = self.parse_item(selector, pattern=pattern)
        return list

    def send_mail_to(self, mail):
        mail = mail.copy()
        if self.debug:
            mail["text"] = self.prepend + mail["text"]
        selector = self.cookie + "bbssndmail?userid="
        pattern = "信件已寄给%s" % mail["userid"]
        v = self.parse_item(selector, mail, pattern)
        v = self.true_or_false(v)
        return v

