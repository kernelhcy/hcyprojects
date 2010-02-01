#! /usr/bin/python
# -*- coding: utf-8 -*-

import sys
from PyQt4.QtGui import *

class CPVMainWindow(QMainWindow):
	def __init__(self):
		QMainWindow.__init__(self)		
		self.setGeometry(0, 0, 550, 550)
		self.setWindowTitle('Cache Pages Viewer')
		self.setWindowIcon(QIcon('icons/web.png'))
		
if __name__ == '__main__':
	args = sys.argv
	app=QApplication(args)
	win=CPVMainWindow()
	win.show()
	sys.exit(app.exec_())
