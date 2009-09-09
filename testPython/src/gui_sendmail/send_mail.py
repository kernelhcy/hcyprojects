# -*- coding:utf-8 -*-

#导入smtplib和MIMEText
import smtplib 
from email.mime.text import MIMEText

class send_Mail:
    def __init__(self):
        
        #收信人列表
        self.to_list = ["huangcongyu2006@qq.com"]
        #设置服务器，用户名、口令以及邮箱的后缀
        self.mail_host = "smtp.qq.com"
        self.mail_user = "huangcongyu2006"
        self.mail_pass = "hcy1988hcy"
        self.mail_from = "huangcongyu2006@qq.com"
        #设置主题和内容
        self.subject="默认"
        self.content="默认"
    
        #设置别名
        self.nikname=""
        
    def set_subject(self,sub):
        """
                设置主题
        """
        self.subject=sub
        return
    def set_content(self,con):
        '''
                设置信件内容
        '''
        self.content=con
        return 
    def set_host(self,host):
        """
                设置邮件服务器
        """
        self.mail_host=host
        return 
    def set_user_pwd(self,user,pwd):
        '''
                设置用户名和密码
        '''
        self.mail_user=user
        self.mail_pass=pwd
        return 
    
    def set_nikname(self,nik):
        '''
                设置发信人的别名
        '''
        self.nikname=nik
        return 
    def set_from(self,mail_from):
        '''
                设置发信人地址
        '''
        self.mail_from=mail_from
        return 
    
    def set_to_list(self,to_list):
        '''设置收信人地址'''
        self.to_list.append(to_list)
        return
    
    def send_mail(self):
        '''
                发送邮件
        '''
        me = self.nikname+"<" + self.mail_from + ">"
        msg = MIMEText(self.content,"html","utf-8")
        
        msg['Subject'] = self.subject
        msg['From'] = me
        msg['To'] = ";".join(self.to_list)
        #    print "邮件信息："
        #    print msg.as_string()
        #    print ""
        try:
            print "连接邮件服务器..."
            s = smtplib.SMTP()
            s.connect(self.mail_host)
            print "登录用户..."
            s.login(self.mail_user, self.mail_pass)
            
            print "发送邮件..."
            s.sendmail(me, self.to_list, msg.as_string())
            s.close()
            return True,"发送成功！"
        except Exception, e:
            print str(e)
            return False,"发送失败...\n"+str(e)