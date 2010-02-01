#! /usr/bin/python
# -*- coding: utf-8 -*-

from searcher import Searcher

"""
	根据获得的文件列表，生成索引文件index.html
"""
class HTMLFile(object):
	def __init__(self, file_list):
		self.__file_list = file_list
		self.__html_str = ''
		self.__scer = Searcher(file_list)
		
	def __add_a(self, link):
		self.__html_str += '<a href="'
		self.__html_str += link
		self.__html_str += '">'
		
		index = link.rfind('.html')
		if index != -1:
			link = link[0: index]
		self.__html_str += link
		self.__html_str += '</a><br/>\n'
	
	def __add_index_a(self, pos):
		self.__html_str += '<a href="#'
		self.__html_str += pos
		self.__html_str += '">'
		self.__html_str += pos
		self.__html_str += '</a>\n'
	
	def create(self):
		#创建html文件头
		self.__html_str += '<html> <head><meta http-equiv=Content-Type content="text/html; charset=utf-8">'
		self.__html_str += '<title>index of cache pages</title></head><br/>\n<body>\n'
		self.__html_str += '<center><h2>Index of Cache Pages</h2></center>\n'
		
		#创建索引
		self.__html_str += '<br/>'
		keywords = self.__scer.get_keywords()
		for kw in keywords:
			self.__add_index_a(kw)
		self.__html_str += '<br/><br/><br/>'
		
		#添加所有文件的连接
		self.__html_str += '<br/> 所有文件：<br/>'
		self.__file_list.sort()
		for t in self.__file_list:
			self.__add_a(t)
		self.__html_str += '<br/>'
		
		#添加关键词分类的结果
		self.__html_str += '<br/> 关键词分类： <br/>'
		re = self.__scer.search()
		for kw in re:
			self.__html_str += '<br/>'
			self.__html_str += '<a name = "'
			self.__html_str += kw
			self.__html_str += '"></a>-----'
			self.__html_str +=  kw
			self.__html_str += '-----------------------------------------------<br/>'
			for t in re[kw]:
				self.__add_a(t)
			self.__html_str += '<br/>'
		#文件尾
		self.__html_str += '</body>\n</html>\n'
		
		return self.__html_str
