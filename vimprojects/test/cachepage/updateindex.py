#! /usr/local/bin/python3
# -*- coding: utf-8 -*-

from xml.dom import minidom
from os.path import exists, sep
#
#建立索引，便于查阅
#

class UpdateIndex:
	def __init__(self, base):
		print("inti UpdateIndex class.")
		self.__base_dir = base
		if self.__base_dir[-1] != sep:
			self.__base_dir += sep
		self.__path = self.__base_dir + 'index.html'
		self.__create_index()
		
	def update(self, filename):
		pass
	
	def __create_index(self):
		
		xmlfile = open(self.__path, 'a+')
		xmldoc = minidom.parse(xmlfile)
		xmlfile.close()
		
		root = xmldoc.documentElement
		
		
		print(root.toxml())
