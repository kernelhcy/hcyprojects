/*********************************
 @author:zhangliha
 @home  :www.zhanglihai.com
 @email :zhanglihai.com@gmail.co
 @date  :2006/10/0
**********************************/
#ifndef __BASE64_H
#define __BASE64_H
#include <stdio.h>
#include <malloc.h>	

void base64_encoder(const char *input,size_t len,char** out_str);


//дʱ��fout�ƶ������ļ���� ��ʼ׷��
//fin�� ������ļ�
void base64_encoder_file(FILE *fin,FILE *fout);	


void base64_decoder(const char *input,size_t len,char** out_str);
	
#endif	//__BASE64_H
	
    