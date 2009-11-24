#include "jni.h"
#include "getdata.h" 

typedef unsigned long  DWORD;
typedef unsigned short WORD;

JNIEXPORT jintArray JNICALL Java_getdata_calculate_1by_1MMX
  (JNIEnv * env, jclass cla, jintArray pic1, jintArray pic2, jint rate){
DWORD h_a,h_b,h_temp,end;

WORD rate1_1=(WORD)(rate*32768/100);
WORD rate1[4]={rate1_1,rate1_1,rate1_1,rate1_1}; 
WORD rate2_1=(WORD)(32767-rate1_1);
WORD rate2[4]={rate2_1,rate2_1,rate2_1,rate2_1}; 

jint length=(*env)->GetArrayLength(env,pic1);//得到数组的长度
jintArray pic_temp=(*env)->NewIntArray(env,length);//定义一个中介数组
int* a=(*env)->GetIntArrayElements(env,pic1,NULL);
int* b=(*env)->GetIntArrayElements(env,pic2,NULL);
int* temp=(*env)->GetIntArrayElements(env,pic_temp,NULL);

h_a = (DWORD)a;
h_b = (DWORD)b;
h_temp= (DWORD)temp;
end=h_temp+length*sizeof(int);


for(;h_temp<end;){

_asm{

mov esi , [h_a]
mov edx ,[h_b]
mov edi , [h_temp]
movd mm0 , [esi]
movd mm1 ,[edx]
pxor mm7 ,mm7
movq mm2 , [rate1]
punpcklbw mm0,mm7
punpcklbw mm1,mm7
psubw mm0 ,mm1
pmulhw mm0,mm2
paddw mm0,mm1
packuswb mm0,mm7
movd [edi],mm0

}
h_a+=4;
h_b+=4;
h_temp+=4;

}
_asm EMMS

(*env)->ReleaseIntArrayElements(env,pic1,a,0);
(*env)->ReleaseIntArrayElements(env,pic2,b,0);
(*env)->SetIntArrayRegion(env,pic_temp,0,length,temp);

return pic_temp;
}