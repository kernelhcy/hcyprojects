# -*- coding:utf-8 -*-
#!/usr/bin/python
print "No.1 :", 2 ** 38

#No. 2
import string
from_str = "abcdefghijklmnopqrstuvwxyz"
to_str = "cdefghijklmnopqrstuvwxyzab"
str = """g fmnc wms bgblr rpylqjyrc gr zw fylb. rfyrq ufyr amknsrcpq ypc dmp. 
bmgle gr gl zw fylb gq glcddgagclr ylb rfyr'q ufw rfgq rcvr gq qm jmle. 
sqgle qrpgle.kyicrpylq() gq pcamkkclbcb. lmu ynnjw ml rfc spj. 
"""
url = "map"
#print str.translate(string.maketrans(from_str,to_str))
print url.translate(string.maketrans(from_str, to_str))

#No.3
file = open('3', 'r')
print ''.join([c for c in file.read() if c.isalpha()])
file.close()

#No.4
file = open('4', 'r')
import re
result = re.findall(r'[^A-Z][A-Z]{3}([a-z])[A-Z]{3}[^A-Z]', file.read())
file.close()
print ''.join(result)

#import re, urllib
#text = urllib.urlopen('http://www.pythonchallenge.com/pc/def/equality.html').read().replace('\n','')
#text = re.findall('<!--(\w*)-->',text)[0]
#print ''.join(re.findall('[a-z][A-Z]{3}([a-z])[A-Z]{3}[a-z]', text))


#No.5
import urllib2, re
url = "http://www.pythonchallenge.com/pc/def/linkedlist.php?nothing="
nothing = "1234"
cnt = 0;
while(True):
    try:
        url_src = urllib2.urlopen(url + nothing, None)
        text = url_src.read()
        nothing = ''.join(re.findall(r'[0-9]', text))
        print text
        print nothing 
        cnt += 1
        print cnt
    except:
        print "result: ", nothing
    
