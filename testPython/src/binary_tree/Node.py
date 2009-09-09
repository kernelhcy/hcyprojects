class Node:
    """
    define the node of the bst
    """
    def __init__(self,key=0,left=None,right=None):
        self.key=key
        self.left=left
        self.right=right
    
    def __cmp__(self,argv):
        "compare the key"
        if argv==None:
            return 0
        
        if isinstance(argv, Node):
            return argv.key<self.key
        else :
            return 0
        
    def __str__(self):
        return "key:"+str(self.key)
    
    
    def set_key(self,key):
        "set the value of key"
        self.key=key
    
    def set_right(self,right=None):
        "set the value of the right child"
        self.right=right
        
    def set_left(self,left=None):
        "set the value of the left child"
        self.left=left
    