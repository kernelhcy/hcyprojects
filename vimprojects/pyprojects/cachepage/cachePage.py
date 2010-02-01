#! /usr/local/bin/python3
# -*- coding: utf-8 -*-

#
# 功能： 
# 抓取一个网页的内容。也就是首先浏览器的另存为全部的功能。
# 保存的同时申城索引文件，便于查阅。
#

from sys import argv
from os import renames, makedirs, sep #sep: 操作系统的路径分割符
from os.path import dirname, exists, isdir, splitext, basename
from urllib.request import urlretrieve
from urllib.parse import urlparse
from optparse import OptionParser

from parsehtml import MyParser
from updateindex import UpdateIndex

class Retriver(object):
	"""
		下载页面内容
	"""
	def __init__(self, url, base = 'cachepages', out = None):
		self.__base_dir = '.' + sep + base + sep
		self.__out = out
		parsedurl = urlparse(url, 'http:', 0)
		self.__netloc = parsedurl[0] + '://' + parsedurl[1]
		self.__path = parsedurl[2]
		print("netloc : %s" % self.__netloc)
		print("path : %s" % self.__path)
		
	def __download(self, path, filename):  # download Web page
		if path == None:
			path = self.__path 
		
		url = self.__netloc + path
		try: 
			print('downloading... %s' % url)
			retval = urlretrieve(url, filename) 
		except IOError: 
			retval = ('*** ERROR: invalid URL "%s"' % url,) 
		return retval
	
	def do(self):
		retval = self.__download(None, 'test.html')
		print('saved as : %s .' % retval[0])
		
		parser = MyParser()
		parser.feed(open('test.html').read())
		parser.close()
		title = parser.get_title()
		
		if self.__out == None:
			self.__out = title
		renames('test.html', self.__base_dir + self.__out + '.html')
		
		links = parser.get_links()
		makedirs(self.__base_dir + self.__out + sep)
		for link in links:
			retval = self.__download(link, self.__base_dir + self.__out + sep + basename(link))
			print('saved as : %s .' % retval[0])

class Main:
	def __init__(self, argv):
		print("init Main class")
		
		self.__argv = argv[1:]
		self.__out = None
		self.__cmd_parser = OptionParser("usage:cachePage [options] title url");
		self.__cmd_parser.add_option('-o', '--out', help = 'Output file name',
							action = 'store', type = 'string', dest = 'out')
		
		self.__execCMD()
		
	def get_page(self):
		r = Retriver(self.__url, 'cachepages', self.__out)
		r.do()
		
	def __execCMD(self):
		options, args = self.__cmd_parser.parse_args(self.__argv)
		self.__url = args[0]
		
		if options.out:
			self.__out = options.out
				
if __name__ == '__main__':
	
	#main = Main(argv)
	#main.get_page()
	ui = UpdateIndex('.')
