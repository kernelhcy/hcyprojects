# -*- coding:utf-8 -*-

#导入smtplib和MIMEText
import smtplib 
from email.mime.text import MIMEText

#收信人列表
mailto_list = ["huangcongyu2006@gmail.com"]

#设置服务器，用户名、口令以及邮箱的后缀
mail_host = "smtp.qq.com"
mail_user = "huangcongyu2006"
mail_pass = "hcy1988hcy"
mail_postfix = "qq.com"


def send_mail(to_list, sub, content):
    '''
    to_list:收信人
    sub:主题
    content:内容
    send_mail("aaa@126.com","sub","content")
    '''
    me = "梧桐雨<" + mail_user + "@" + mail_postfix + ">"
    msg = MIMEText(content,"html","utf-8")
    
    msg['Subject'] = sub
    msg['From'] = me
    msg['To'] = ";".join(to_list)
#    print "邮件信息："
#    print msg.as_string()
#    print ""
    try:
        print "连接邮件服务器..."
        s = smtplib.SMTP()
        s.connect(mail_host)
        print "登录用户..."
        s.login(mail_user, mail_pass)
        
        print "发送邮件..."
        s.sendmail(me, to_list, msg.as_string())
        s.close()
        return True
    except Exception, e:
        print str(e)
        return False
if __name__ == '__main__':
    content="Python  <br> <h1>邮件内容。</h1>"
    subject="邮件主题－－python"
    if send_mail(mailto_list, subject, content):
        print "发送成功！"
    else:
        print "发送失败。。。"