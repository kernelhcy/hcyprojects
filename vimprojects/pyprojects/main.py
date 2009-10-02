f=open("temp","w")
for i in range(100):
    f.write("%dtemp"%i)
    print f.newlines
    #f.writenewline()
f.close()

if __name__=="__main__":
    print "This is in main!"
    f=open("temp","r+")
    #lines=f.readlines()
    for line in f:
        print line
    f.close()
