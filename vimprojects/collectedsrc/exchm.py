#!/usr/bin/python
'''
CHM2PDF

A script that converts a CHM compiled HTML file into a single PDF file.
(c) Massimo Sandal, 2007

Released under the GNU GPL version 2 or later.
'''

import chm.chm as chm
import sys
import sgmllib
import os

class PageLister(sgmllib.SGMLParser):
    '''
    parser of the chm.chm GetTopicsTree() method that retrieves the URL of the HTML
    page embedded in the CHM file.
    '''
    def reset(self):
        sgmllib.SGMLParser.reset(self)
        self.pages=[]
        
    def start_param(self,attrs):
       urlparam_flag=False
       for key,value in attrs:
           if key=='name' and value=='Local':
               urlparam_flag=True
           if urlparam_flag and key=='value':
               self.pages.append('/'+value)

class Exchm:
	def __init__(self,filename):
		self.infile=filename;  
		print "File Name ", self.infile
		print chm
		self.cfile=chm.CHMFile()
		self.cfile.LoadCHM(self.infile)
		print "Load done."
    	
	def get_html_list(self,cfile):
		'''
		retrieves the list of HTML files contained into the CHM file, in order.
		(actually performed by the PageLister class)
		'''
		self.topicstree=cfile.GetTopicsTree()
		print 'topicstree: ', self.topicstree
		self.lister=PageLister()
		self.lister.feed(self.topicstree)
		
	def extend(self):
		self.get_html_list(self.cfile)
		self.html_list=self.lister.pages
		#print 'list: ',self.html_list 
		self.TEMP_DIR=self.infile+"_files"
    	
		try:
			os.mkdir(self.TEMP_DIR)
		except OSError: #there is already the directory
			pass
		os.chdir(self.TEMP_DIR)
    	
		self.c=0
		for url in self.html_list:
			self.c+=1
			self.ui=self.cfile.ResolveObject(url)[1]
			self.page=self.cfile.RetrieveObject(self.ui)[1]
    	    
			self.f=open("."+url,'w')
			self.f.write(self.page)
			self.f.close
    	    
		print 'Done.'
		self.cfile.CloseCHM()
    	
def help():
	print 'Usage: exchm [input filename]'

def main(argv):
    
	if len(argv)==1:
		help()
		return
	else:
		filename = argv[1]
	exchm=Exchm(filename)
	exchm.extend()


if __name__ == '__main__':
		main(sys.argv)
    
