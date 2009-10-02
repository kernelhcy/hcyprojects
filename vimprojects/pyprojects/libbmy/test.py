# -*- coding: UTF-8 -*- 

import libbmy
import sys

con = libbmy.client()

r = con.login('hcy', 'hcy1988hcy')

if r:
    print("登录成功")
else:
    print("登录失败")
    sys.exit()

art = {
        "title":    "测试libbmy",
        "text":     "^-^",
        "board":    "linux"
      }  

r = con.post_force(art)

if r:
    print("发贴成功")
else:
    print("发贴失败")

con.logout()

