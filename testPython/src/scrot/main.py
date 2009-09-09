# -*- coding:utf-8 -*-
#!/usr/bin/python2.5
import os
print "begin..."
os.system('/usr/bin/scrot -cd 1 -q 85 -s -b -e \'mv $f ~/screenshots/ss\'')
print "over!"