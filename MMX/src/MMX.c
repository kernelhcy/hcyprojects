#include "jni.h"
#include "getdata.h" 

JNIEXPORT jintArray JNICALL Java_getdata_calculate_1by_1MMX
  (JNIEnv * env, jclass cla, jintArray pic1, jintArray pic2, jint rate){
jlong i=0;

jint length=(*env)->GetArrayLength(env,pic1);//得到数组的长度

jintArray pic_temp=(*env)->NewIntArray(env,length);//定义一个中介数组
int* a=(*env)->GetIntArrayElements(env,pic1,NULL);
int* b=(*env)->GetIntArrayElements(env,pic2,NULL);
int* temp=(*env)->GetIntArrayElements(env,pic_temp,NULL);



for(i=0;i<length;i++)
{
	temp[i]=(a[i]*rate+b[i]*(100-rate))/100;
	
	

}

(*env)->ReleaseIntArrayElements(env,pic1,a,0);
(*env)->ReleaseIntArrayElements(env,pic2,b,0);
(*env)->ReleaseIntArrayElements(env,pic_temp,temp,0);

return pic_temp;
}