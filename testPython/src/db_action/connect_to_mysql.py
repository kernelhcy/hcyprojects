# -*- coding:utf-8 -*-
import sys
import MySQLdb
conn=MySQLdb.connect(host='localhost',user='hcy',passwd='19hcy88',db='test',charset='utf8')
cur=conn.cursor()

num=cur.execute('select * from test')
print 'The number of the data in the database is:%s'%(num,)

#data=cur.fetchall()
#print 'The data after insert:'
#for i in data:
#    print i[0],i[1],i[2],i[3]

insert_sql="insert into test (name,sex,school) values ('王','男','西北工业大学')"

for i in range(10):
    pass
    #cur.execute(insert_sql)
data=cur.fetchall()
print 'The data after insert:'
for i in data:
    print i[0],i[1],i[2],i[3]
delete_sql="delete from test"
#cur.execute(delete_sql)
try:
    cur.close()
    conn.close()
    #help ('MySQLdb')
    raise MySQLdb.MySQLError,'test error...'
except MySQLdb.MySQLError,e:
    print e