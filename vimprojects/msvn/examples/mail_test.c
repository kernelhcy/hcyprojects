/*********************************
 @author:zhangliha
 @home  :www.zhanglihai.com
 @email :zhanglihai.com@gmail.co
 @date  :2006/10/0
**********************************/
#include <stdio.h>
#include "base64.h"
#include "send_mail.h"	
	
void test_mail(){
    int r =0;
    struct st_char_arry to_addrs[1];
  	to_addrs[0].str_p="zhanglihai.com@gmail.com";
    struct st_char_arry att_files[2];
    att_files[0].str_p="/home/cheung/workspace/cpp/send_mail/QQ.png";
    att_files[1].str_p="/home/cheung/workspace/cpp/send_mail/send_mail.h";
	struct st_mail_msg_ mail;
	init_mail_msg(&mail);
	//对方服务器要求权限验证
	mail.authorization=AUTH_SEND_MAIL;
	//smtp.163.com
	//ip or server
	mail.server="";
	mail.port=25;
	mail.auth_user="";
	mail.auth_passwd="";
	mail.from="";
	mail.from_subject="no-reply@zhanglihai.com";
	mail.to_address_ary=to_addrs;
	mail.to_addr_len=1;
	mail.content="测试信的内容你们好<b>中文的内容</b>测试信的内容你们好<b>中文的内容</b>"
		"测试信的内容你们好<b>中文的内容</b>测试信的内容你们好<b>中文的内容</b>测试"
		"<font color=red>信的内容</font>你们好<b>中文的内容</b>";
	mail.subject="测试<>标题!!";
	mail.mail_style_html=HTML_STYLE_MAIL;
	mail.priority=3;
	mail.att_file_len=2;
	mail.att_file_ary=att_files;
	r = send_mail(&mail);
	printf("send mail [%d]\n",r);	
	
}

int main(int argc,char *argv[])
{
  	test_mail();
 	return 0;	
}
