class Test:
    __a=0
    def __init__(self):
       self.b=0
       self.__a=self.__a+1
       #Test.__a=Test.__a+1
    def get_a(self):
        return self.__a
test1=Test()
test2=Test()
print 'test1'
print 'a:%d'%(test1.get_a())
test1.a=10
print 'test2'
print 'a:%d'%(test2.get_a())