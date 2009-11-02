/* 
 * File:   my_alloc.h
 * Author: hcy
 *
 * Created on 2009年8月13日, 下午10:27
 */

#ifndef _MY_ALLOC_H
#define	_MY_ALLOC_H

#ifdef	__cplusplus
extern "C"
{
#endif

#include <unistd.h>
#define INIT_S 10240 //堆区的默认大小
    int is_initialized = 0; //标记是否已经初始化
    void *memory_start; //堆区的开始地址
    void *memory_end; //系统中断点

    /*
     * 内存控制块
     * 块头和块尾的结构
     */
    typedef struct __mem_control_block
    {
        int available; //是否可用
        int size; //大小
    } mem_control_block;

    /*
      * 初始化堆区。
      * 建立堆区的初始结构，设置序言块，结尾块等。
      */
    void my_malloc_init()
    {
        /*
         * 获得系统中断点，也即堆区的堆顶
         */
        memory_end = sbrk(0);
        /*
         * 堆区的开始位置
         */
        memory_start = memory_end;
        /*
         * 为堆区预先分配INIT_S个字节。
         */
        if (sbrk(INIT_S) <= 0)
        {
            printf("Init Error!\n");
            return;
        }
        memory_end += INIT_S;

        mem_control_block *mcb;
        /*序言块*/
        /*块头*/
        mcb = memory_start;
        mcb -> available = 0;
        mcb -> size = 0;
        /*块尾*/
        mcb = (void*)mcb+ sizeof(mem_control_block);
        mcb -> available = 0;
        mcb -> size = 0;
        
        /*设置控制块， 块头*/
        mcb = memory_start + 2 * sizeof(mem_control_block);
        mcb -> available = 1;
        /*这个块的长度为整个堆区（除去控制块的长度）*/
        mcb -> size = INIT_S - 5 * sizeof (mem_control_block);
        /*设置边界标记，块尾*/
        mcb = memory_end - 2 * sizeof (mem_control_block);
        mcb -> available = 1;
        mcb -> size = INIT_S - 5 * sizeof (mem_control_block);


        /*结尾块*/
        mcb = memory_end - sizeof (mem_control_block);
        mcb -> available = 0;
        mcb -> size = 0;

        /*
         * 已经进行了初始化
         */
        is_initialized = 1;
    }

    /*
      * 模拟malloc函数
      */
    void * my_malloc(int size)
    {
        if (!is_initialized)
        {
            my_malloc_init();
        }

        /*分配的内存块的地址*/
        void * mem_location = 0;

        /*内存控制块的指针*/
        mem_control_block *curr_mcb, *tail;

        /*从头开始遍历*/
        curr_mcb = memory_start;

        while ((void*)curr_mcb < memory_end)
        {
            if (curr_mcb -> available)/*找到一个空闲块*/
            {
                if (curr_mcb -> size >= size && curr_mcb -> size < size + 2 * sizeof (mem_control_block))
                    /*大小合适，多余的空间不够进行分割的。也就是剩余的空间无法满足块头和块尾所需要的空间。*/
                {
                    /*获得返回的内存地址*/
                    /*
                          *	 此处必须把curr_mcb转成void*的格式！！
                          *  否则，在加的时候是以sizeof(mem_control_block)的倍数增加，而不是仅仅加一！！
                          */
                    mem_location = (void *) curr_mcb + sizeof (mem_control_block);
                    /*标记已经占用*/
                    curr_mcb -> available = 0;

                    /*获得边界标记的指针，块尾*/
                    tail = (void *) curr_mcb + sizeof (mem_control_block) + curr_mcb -> size;

                    /*标记已经占用*/
                    tail -> available = 0;
                    break;
                }
                else if (curr_mcb -> size > size + 2 * sizeof (mem_control_block))
                    /*进行分割*/
                {
                    int old_size = curr_mcb -> size;
                    /*获得分配的内存块的地址*/
                    mem_location = (void *) curr_mcb + sizeof (mem_control_block);
                    /*标记已经占用*/
                    curr_mcb -> available = 0;
                    curr_mcb -> size = size;
                    /*获得边界标记的指针，块尾*/
                    tail = (void *) curr_mcb + sizeof (mem_control_block) + size;
                    /*标记已经占用*/
                    tail -> available = 0;
                    tail -> size = size;

                    /*将余下的部分分割成新的空闲块*/
                    mem_control_block *hd, *tl; /*新块的块头和块尾*/
                    /*块头*/
                    hd = (void *) tail + sizeof (mem_control_block);
                    hd -> available = 1;
                    hd -> size = old_size - size - 2 * sizeof (mem_control_block);
                    /*块尾*/
                    tl = (void *) hd + hd -> size + sizeof (mem_control_block);
                    tl -> available = 1;
                    tl -> size = hd -> size;

                    break;
                }
            }

            /*指向下一个块*/
            curr_mcb = (void*) curr_mcb + curr_mcb -> size + 2 * sizeof (mem_control_block);
        }

        /*没有找到合适的块，则扩展堆区，分配合适大小的内存加到堆区*/
        if (!mem_location)
        {
            /*申请空间*/
            if (sbrk(size + 2 * sizeof (mem_control_block)) <= 0)
            {
                printf("Sbrk Error!\n");
                return 0;
            }
            /*设置控制块的信息*/
            curr_mcb = (void*)memory_end - sizeof(mem_control_block);
            curr_mcb -> available = 0;
            curr_mcb -> size = size;
            /*设置边界标记，块尾的信息*/
            tail = (void*) curr_mcb + curr_mcb -> size + sizeof (mem_control_block);
            tail -> available = 0;
            tail -> size = size;

            /*获得分配的内存块的地址*/
            mem_location = (void*)curr_mcb + sizeof (mem_control_block);

            memory_end =  memory_end + size + 2 * sizeof (mem_control_block);
             /*设置结尾块*/
            tail = (void*)memory_end - sizeof(mem_control_block);
            tail -> available = 0;
            tail -> size = 0;
        }

        return mem_location;
    }
    /*
     * 模拟free函数
     */
    void my_free(void *ptr)
    {
        if (ptr <= 0)
        {
            return;
        }

        mem_control_block *curr;
        /*指向控制块的地址*/
        curr = ptr - sizeof (mem_control_block);

        

        /*标记为空闲，可用*/
        curr->available = 1;
        /*
         * 合并
         */
        mem_control_block *pre, *next, *tmp;
        
        /*获得前一个块的块头地址*/
        pre = ptr - 2 * sizeof (mem_control_block);/*这条语句获得了前一个块的块尾的地址*/
        pre = (void *) pre - pre -> size - sizeof (mem_control_block);/*进一步计算块头的地址*/

        /*获得后一个块的块头地址*/
        next = ptr + curr -> size + sizeof (mem_control_block);

        if (!pre -> available && next -> available)/*只有后一个块空闲*/
        {
            curr -> size += (next -> size + 2 * sizeof (mem_control_block));
            /*设置块尾*/
            tmp = (void *) curr + curr -> size + sizeof (mem_control_block);
            tmp -> available = 1;
            tmp -> size = curr -> size;
        }
        else if (pre -> available && !next -> available)/*只有前一个块空闲*/
        {
            pre -> size += (curr -> size + 2 * sizeof (mem_control_block));
            /*设置块尾*/
            tmp = (void *) pre + pre -> size + sizeof (mem_control_block);
            tmp -> available = 1;
            tmp -> size = pre -> size;
        }
        else if (pre -> available && next -> available)/*前后都块空闲*/
        {
            pre -> size += (curr -> size + 4 * sizeof (mem_control_block) + next -> size);
            /*设置块尾*/
            tmp = (void *) pre + pre -> size + sizeof (mem_control_block);
            tmp -> available = 1;
            tmp -> size = pre -> size;
        }

        return;

    }

    void print_info()
    {
        printf("printf_info:\n");
        mem_control_block *curr = memory_start;
        while ((void*)curr < memory_end)
        {

            printf("a? %d s: %d %d %d\n", curr ->available, curr ->size, memory_end, curr);
            curr = (void*) curr + curr -> size + 2 * sizeof (mem_control_block);
        }
    }


#ifdef	__cplusplus
}
#endif

#endif	/* _MY_ALLOC_H */

