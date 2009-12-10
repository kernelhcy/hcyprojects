#include <stdlib.h>
#include <stdio.h>
#include <string.h>

//用char来表示byte
char S[256]; 	//状态矢量
char T[256]; 	//临时矢量
char K[256]; 	//密钥 
int keylen = -1;

void swap(char *, char *);

void init(const char *key)
{
	keylen = strlen(key);
	printf("Init...\nThe length of the key is %d\n", keylen);

	memset(K, 0, sizeof(K));
	strcpy(K, key);
	int i = 0;
	for (i = 0; i < 256; ++i)
	{
		S[i] = (char)i;
		T[i] = K[i % keylen];
	}

	int j = 0;
	for (i = 0; i < 256; ++i)
	{
		j = (j + S[i] + T[i]) % 256;
		swap(S + i, S + j);
	}
}

/**
 * 加解密数据流。
 * @Parm data 数据
 * @Parm len 长度
 */
void crypt(char *data, int len)
{
	char k;
	int i, j, t;
	int ndx = 0;
	i = 0;
	j = 0;

	for (ndx = 0; ndx < len; ++ndx )
	{
		i = (i + 1) % 256;
		j = (j + S[i]) % 256;
		swap(S + i, S + j);
		t = (S[i] + S[j]) % 256;
		k = S[t];

		data[ndx] = data[ndx] ^ k;
	}

}

int main(int argc, char *argv[])
{
	
	if (argc < 3)
	{
		printf("Please input data and key!\n");
		return 1;
	}
	
	int len = strlen(argv[1]);
	char data[len + 1];
	strcpy(data, argv[1]);
	data[len] = '\0';

	char *key = argv[2];
	if (strlen(key) > 256)
	{
		printf("The length of the key must be less than 256!!\n");
		return 1;
	}

	printf("Data: %s\nkey: %s\n", data, key);
	init(key);
	
	printf("Encrypting...\n");
	crypt(data, len);
	printf("After encrypt.\nData is %s \n", data);
	printf("Show data in unsigned integer.\n");
	int i;
	for (i = 0; i < len; ++i)
	{
		printf("%u ", (unsigned char)data[i]);
	}
	printf("\n");

	init(key);
	printf("Decrypt...\n");
	crypt(data, len);
	printf("After decrypt.\nData is %s\n", data);
	
	return 0;
}


void swap(char *a, char *b)
{
	char c;
	c = *a;
	*a = *b;
	*b = c;
}
