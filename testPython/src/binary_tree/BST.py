# -*- coding:utf-8 -*-
#!/usr/bin/python
from Node import Node
class BST:
    
    def __init__(self):
        """
        init the bst
        
        """
        
        print "init!"
        
        self.root=None
        
    def insert(self,key):
        
        #print "insert %d into the bst"%(key,)
        self.root=self.__insert_help(self.root, key)
        
    
    def __insert_help(self,root,key):
        "insert a key into the bst"
        
        if root is None:
            #print "root is None"
            root=Node(key)
            #print root
        else:
            if root.key>key:
                root.set_right(self.__insert_help(root.right,key))
                #print "insert %d into the right"%(key,)
            else:
                root.set_left(self.__insert_help(root.left,key))
                #print "insert %d into the left"%(key,)
        return root
    
    def mid_travel(self):
        "中序遍历"
        self.__mid_travel_help(self.root)
    
    def __mid_travel_help(self,root):
        
        if root is None:
            return
        self.__mid_travel_help(root.right)
        print root.key
        self.__mid_travel_help(root.left)
