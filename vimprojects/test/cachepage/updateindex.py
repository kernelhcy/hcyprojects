#! /usr/local/bin/python3
# -*- coding: utf-8 -*-

"""
	将保存的网页存放在一个固定的目录中。这个程序定期扫描目录，
	当有新的文件加入后，生成索引文件index.html。便于查阅
"""

from os import listdir, makedirs, chdir, getcwd
from os.path import exists, isdir, sep

class UpdateIndex(object):
	def __init__(self, base):
		if base[-1] != sep:
			self.__base_dir = base + sep
		else:
			self.__base_dir = base
		
		self.__cache_file_name = '.cachefilelist' #缓存文件列表文件
		self.__index_file_name = 'index.html'	 #索引文件
		
		if not exists(base):
			print("First run. create directory %s" % base)
			makedirs(base)
		self.__now_files = [] 		#文件列表
		self.__cache_files = [] #上次缓存的文件列表
		
		
	"""
		读取base目录下的所有html文件名，不包含目录的名称。
		也不包含文件缓存文件和索引文件
	"""
	def __list_files(self):
		#读取目录下的所有文件。
		files = listdir(self.__base_dir)
		old_dir = getcwd()
		chdir(self.__base_dir)
		for f in files:
			if not isdir(f) \
			   and f != self.__cache_file_name \
			   and f != self.__index_file_name:
				self.__now_files.append(f)
	
		chdir(old_dir)
		
	"""
		获得缓存的列表，以判断是否有新文件加入.
	"""
	def __get_cache_list(self):
		if not exists(self.__cache_file_name):
			#创建缓存文件
			open(self.__cache_file_name, 'w').close()
		
		cf = open(self.__cache_file_name, 'r')
		try:
			self.__cache_files = [v for v in cf]
		finally:
			cf.close()
	
	"""
		更新索引
	"""
	def __update_index(self):
		#更新缓存文件
		cf = open(self.__cache_file_name, 'w')
		try:
			for f in self.__now_files:
				cf.write(f)
				cf.write('\n')
		finally:
			cf.close()
		
		htmlcreater = HTMLFile(self.__now_files)
		htmlstr = htmlcreater.create()
		f = open(self.__index_file_name, 'w')
		f.write(htmlstr)
		f.close()
		
	"""
		判断是否有新的文件加入
	"""
	def __have_new(self):
		if len(self.__cache_files) == 0 and len(self.__now_files) != 0 :
			return True
		
		for var in self.__now_files:
			if var not in self.__cache_files:
				return True
		
		return False
		
		
	"""
		调用此函数，更新索引
	"""
	def update(self):
		self.__list_files()
		chdir(self.__base_dir) 	#切换目录
		self.__get_cache_list()
		if self.__have_new():
			self.__update_index()
	"""
		输出所读取到的文件名
	"""
	def __show_files(self):
		for f in self.__now_files:
			print(f)
"""
	根据获得的文件列表，生成索引文件index.html
"""
class HTMLFile(object):
	def __init__(self, file_list):
		self.__file_list = file_list
		self.__html_str = ''
		
	def __add_a(self, link):
		self.__html_str += '<a href='
		self.__html_str += link
		self.__html_str += '>'
		self.__html_str += link
		self.__html_str += '</a><br/>\n'
	
	def create(self):
		self.__html_str += '<html> <head><title>index of cache pages</title></head><br/>\n<body>\n'
		self.__html_str += '<center><h2>Index of Cache Pages</h2></center>\n'
		for link in self.__file_list:
			self.__add_a(link)
		self.__html_str += '</html>\n'
		
		return self.__html_str

if __name__ == '__main__':
	ui = UpdateIndex('cachepages')
	ui.update()
	
	
