# -*- coding:utf-8 -*-
#!/usr/bin/python
class Equation:
    """
    Equation class 
    solve a equation like ax2+bx+c=0
    """
    def __init__(self, a=0, b=0, c=0):
        "init the equation"
        self.a = a
        self.b = b
        self.c = c
        self.deta = b * b - 4 *a* c
    def set_a(self, a):
        "set the value of a"
        "a must not be zero"
        assert a!=0 
        self.a = a
    def set_b(self, b):
        "set the value of b"
        self.b = b
    def set_c(self, c):
        "set the value of c"
        self.c = c
    def __str__(self):
        "print the equation"
        s = str(self.a)
        s += " * x2 + "
        s += str(self.b)
        s += " * x + "
        s += str(self.c)
        s += " = 0"
        return s
    def can_solve(self):
        "judge if this equation can be solved"
        return self.deta >= 0
    def solve(self):
        """
        solve this equation
        return a string like x1=?, x2=?
        """
        assert self.a!=0
        
        import math
        te = math.sqrt(self.deta)
        x1 = (- self.b + te) / (2 * self.a)
        x2 = (- self.b - te) / (2 * self.a)
        s = "x1="
        s += str(x1)
        s += ", "
        s += "x2="
        s += str(x2)
        return s
def test(a, b, c):
    equation = Equation(a, b, c)
    print equation
    if equation.can_solve():
        print "This equation can be solved!"
        print "The answer is:"
        print equation.solve()
    else:
        print "Sorry,this equation has no answer...  (b*b-4*a*c = %d)" % (equation.deta,)
if __name__ == "__main__":
    import random
    for i in range(10):
        print "Test:%d" % (i+1,)
        test(int(random.random()*10000)
             ,int(random.random()*10000)
             ,int(random.random()*10000))
        print 
        