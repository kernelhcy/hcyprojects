#! /usr/bin/python
# -*- coding: utf-8 -*-

"""
	根据提供的关键词，查询相应的标题
"""
class Searcher(object):
	def __init__(self, titles):
		self.__titles = titles
		self.__result = {}
		
		"""
			分类的关键词。
			在这里面增加需要的分类。
		"""
		self.__keywords = ("java", "c++", "linux", "javascript", \
							"spring", "gobject", "gcc", "c语言", \
							"其他")
		
		#初始化
		for kw in self.__keywords:
			self.__result[kw]=[]
		
		
	"""
		执行查询，返回查询的结果。
		结果是一个字典。
	"""
	def search(self):
		has_divided = []
		for t in self.__titles:
			for kw in self.__keywords:
				if kw in t.lower():
					self.__result[kw].append(t)
					if t not in has_divided:
						has_divided.append(t)
						
		#对于没有匹配的目录
		#存储的默认的目录中"其他"
		for t in self.__titles:
			if t not in has_divided:
				self.__result[self.__keywords[-1]].append(t)
				
		# sort the result
		for kw in self.__keywords:
			self.__result[kw].sort()
		
		return self.__result

	def get_keywords(self):
		return self.__keywords
