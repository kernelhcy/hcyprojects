/*********************************
 @author:zhangliha
 @home  :www.zhanglihai.com
 @email :zhanglihai.com@gmail.co
 @date  :2006/10/0
**********************************/
#ifndef	__SEND_MAIL_H
#define __SEND_MAIL_H
	
#include <stdio.h>
//html��ʽ����mail
#define	HTML_STYLE_MAIL    0x1
//����mail��Ҫ��֤
#define	AUTH_SEND_MAIL    0x1
	
#define SEND_RESULT_SUCCESS    0
#define SEND_RESULT_OPEN_SOCK_FINAL    0x1	
#define SEND_RESULT_CONNECT_FINAL   0x2
#define SEND_RESULT_FINAL    0x3

#define READ_FILE_LEN	1024

#define PROTOCOL "tcp" 	
	
struct st_char_arry{
  char *str_p;	
};
struct st_mail_msg_{
 //���յ�ַ����
 int to_addr_len;
 int bc_addr_len;
 //���͵�ַ��
 int cc_addr_len; 
 int att_file_len;
 //����Ȩ
 int priority;
 //mail server port.
 int port;
 //�Ƿ���Ҫ��֤
 //1 y,
 int authorization;
 //�����Ƿ���html��ʽ
 int mail_style_html;
  //mail server IP or Host.
 char *server;

  //����
 char *subject;
 //�ı�����
 char *content;

 //��Ҫ��֤���ʺ�
 char *auth_user;
 //��Ҫ��֤������
 char *auth_passwd;
 //�ַ�����
 char *charset;
 
 //���͵�ַ
 char *from;
 //�����˿����ĵ�ַ�������������Ϊfrom
 char *from_subject;
 
 
    //��������
 //file path 
 struct st_char_arry *att_file_ary;
  //����
 struct st_char_arry *cc_address_ary;
  //�����˵�ַ����
 struct st_char_arry *bc_address_ary;
 	//�����˵�ַ
 struct st_char_arry *to_address_ary;
};
//��ʼ���ṹ
void init_mail_msg(struct st_mail_msg_ *msg);
//����mail
int send_mail(struct st_mail_msg_ *msg_);



#endif	//__SEND_MAIL_H
	
	