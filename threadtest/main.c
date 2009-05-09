/* 
 * File:   main.c
 * Author: hcy
 *
 * Created on 2008年10月26日, 上午11:45
 */

#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <time.h>

/*
 * 
 */

void thread(int arg) {
    int i;
    for (i = 0; i < 3; i++) {
        printf("This is a pthread: %d.\n", arg);
        //sleep(1);
    }
}

int main(int argc, char** argv) {

    pthread_attr_t attr;
    pthread_t id;

    int i, ret;
    int a=123;

    /*初始化属性值，均设为默认值*/
    pthread_attr_init(&attr);
    pthread_attr_setscope(&attr, PTHREAD_SCOPE_PROCESS);

    ret = pthread_create(&id, &attr, (void*) thread, (void*)a);
    if (ret != 0) {
        printf("Create pthread error!\n");
        exit(1);
    }

    
    for (i = 0; i < 3; i++) {
        printf("This is the main process.\n");
        pthread_join(id, NULL);
        //sleep(1);
    }
    return (EXIT_SUCCESS);
}

