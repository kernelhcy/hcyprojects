#include "headers.h"
#include <time.h>

//函数声明
void FIFO();
void LRU();
void NUR();
void OPT();
void init();
void clear_memory();
/*
 * 定义数据结构，表示硬盘中的页
 */
struct Page
{
	//该页存放在内存中时，对应的帧号。-1表示不在内存中。
	int frame_id;
	
	//表示该页是否在内存中
	bool is_in_memory;
};

/*
 * 定义数据结构，表示内存中的帧
 */
struct Frame
{
	//当该帧被占用时，占用其的页号。-1表示没有被占用。
	int page_id;

	//表示该帧是否被占用
	bool is_free;

	//记录此帧中的页面已经多长时间没被调用
	//用于LRU算法
	int no_use_time;

	//用于NUR算法
	//表示此帧中的页面在本次时间片中被使用的情况
	bool has_used;
};

//进程页面数,页号是从0到PP-1
const int PP = 10;
//内存分配的帧数
const int AP = 5;
//记录页面总共使用的次数
const int total_instruction = 20;
//总共换人页面的次数
int diseffect = 0;

//表示进程的页
struct Page pages[PP];
//表示内存中分配的帧数
struct Frame pagecontrol[AP];

//待处理的进程的页面顺序
int main_t[total_instruction];

//初始化数据
void init()
{
	diseffect = 0;
	
	for(int i = 0; i < PP; ++i)
	{
		pages[i].frame_id = -1;
		pages[i].is_in_memory = false;
	}

	for(int i = 0; i < AP; ++i)
	{
		pagecontrol[i].page_id = -1;
		pagecontrol[i].is_free = true;
		pagecontrol[i].no_use_time = 0;
		pagecontrol[i].has_used = false;
	}
	
	//初始化页面序列
	srand((int)time(0));
	std::cout << "instructions: ";
	for(int i = 0; i < total_instruction; ++i)
	{
		main_t[i] = rand()%PP;
	//	std::cout << main_t[i] << " ";
	}
	std::cout << std::endl;
}

/*
 * 清除内存帧信息。
 */
void clear_memory()
{
	diseffect = 0;

	for(int i = 0; i < AP; ++i)
	{
		pagecontrol[i].page_id = -1;
		pagecontrol[i].is_free = true;
		pagecontrol[i].no_use_time = 0;
		pagecontrol[i].has_used = false;
	}
	
	for(int i = 0; i < PP; ++i)
	{
		pages[i].frame_id = -1;
		pages[i].is_in_memory = false;
	}
}

/*
 * 显示FIFO算法中，队列中的内容
 */
void FIFO_show_queue(int *queue, int queue_len, int now_page, int head)
{
	
	std::cout <<now_page << " Queue: ";
	for(int j = 0, k = head; j < queue_len; ++j, k = (k + 1) % AP)
	{
		std::cout<<queue[k]<<" ";
	}
	std::cout<<std::endl;
}
/*
 * 模拟先进先出算法
 */
void FIFO()
{
	std::cout<<"FIFO:\n";

	clear_memory();

	int queue[AP];//队列
	int queue_len  = 0;//队列长度
	int head = 0,tail = 0;//队列的头和尾
	
	//清空队列
	memset(queue, 0, sizeof(queue)/sizeof(int));

	for(int i = 0; i < total_instruction; ++i)
	{
		//输出当前队列的信息
		//FIFO_show_queue(&queue[0],queue_len,main_t[i],head);

		if(!pages[main_t[i]].is_in_memory)//此页不在内存中
		{
			++diseffect;
			
			//查看是否有空闲的帧
			bool has_free_frame = false;
			int tmp_id;
			for(tmp_id = 0; tmp_id < AP; ++tmp_id)
			{
				if(pagecontrol[tmp_id].is_free)//有空闲的帧
				{
					has_free_frame = true;
					break;
				}
			}

			if(has_free_frame)//有空闲的帧时
			{
				queue[tail] = main_t[i];//将页号放入队尾
				tail=(tail + 1) % AP;
				++queue_len;//队列长度加一
				
				//更新被占用的帧的信息
				pagecontrol[tmp_id].page_id = main_t[i];
				pagecontrol[tmp_id].is_free = false;
				
				//更新页的信息
				pages[main_t[i]].is_in_memory = true;
				pages[main_t[i]].frame_id = tmp_id;

			}
			else//没有空闲的帧
			{
				//更新页的信息。
				//将页放入队头的页所占用的帧中
				pages[main_t[i]].frame_id = pages[queue[head]].frame_id;
				pages[main_t[i]].is_in_memory = true;

				//将队头的页换出
				pages[queue[head]].frame_id = -1;
				pages[queue[head]].is_in_memory = false;

				//更新帧的信息
				pagecontrol[queue[head]].page_id = main_t[i];
				
				//更新对列，将页加入队尾
				head = (head + 1) % AP;
				queue[tail] = main_t[i];
				tail = (tail + 1) % AP;
			}

		}
		else//此页在内存中，调用之
		{
			continue;
		}//end of if(pages.[main_t[i]].is_in_memory)...
	}
	std::cout << "\t命中率：" << (1 - (float)diseffect/(float)total_instruction)*100 << "%\n";
}

/*
 * 显示LRU算法中内存信息。
 * 输出内存帧中页的id和最长未被使用时间。
 */
void LRU_show_memory(int now_page)
{
	
	std::cout << now_page << " Memory: ";
	for(int j = 0; j < AP; ++j)
	{
		std::cout << pagecontrol[j].page_id << "(" << pagecontrol[j].no_use_time << ")" << " ";
	}
	std::cout << std::endl;
}
/*
 * 模拟LRU算法
 */
void LRU()
{
	std::cout<<"LRU: \n";

	clear_memory();

	for(int i = 0; i < total_instruction; ++i)
	{
		//输出当前内存的使用情况
		//LRU_show_memory(main_t[i]);

		//首先将内存中所有页面的未被使用的时间加一
		//
		for(int j = 0; j < AP; ++j)
		{
			if(!pagecontrol[j].is_free)
			{
				++pagecontrol[j].no_use_time;
			}
		}

		if(!pages[main_t[i]].is_in_memory)//此页不在内存中
		{
			++diseffect;
			
			//查看是否有空闲的帧
			bool has_free_frame = false;
			int tmp_id;
			for(tmp_id = 0; tmp_id < AP; ++tmp_id)
			{
				if(pagecontrol[tmp_id].is_free)//有空闲的帧
				{
					has_free_frame = true;
					break;
				}
			}
			
			if(has_free_frame)//有空闲帧
			{
				pages[main_t[i]].is_in_memory = true;
				pages[main_t[i]].frame_id = tmp_id;

				pagecontrol[tmp_id].is_free = false;
				pagecontrol[tmp_id].page_id = main_t[i];
			}
			else//没有空闲帧，替换页
			{
				//寻找最长时间未被使用的页面
				int longest_id = -1;
				int longest_time = -1;
				for(int j = 0; j < AP; ++j)
				{
					if(!pagecontrol[j].is_free && pagecontrol[j].no_use_time > longest_time)
					{
						longest_time = pagecontrol[j].no_use_time;
						longest_id = j;
					}
				}
				//更新被替换出去的页面的信息
				pages[pagecontrol[longest_id].page_id].is_in_memory = false;
				pages[pagecontrol[longest_id].page_id].frame_id = -1;
				//替换页面
				pagecontrol[longest_id].no_use_time = 0;
				pagecontrol[longest_id].page_id = main_t[i];
				//更新此页面的信息
				pages[main_t[i]].is_in_memory = true;
				pages[main_t[i]].frame_id = longest_id;

			}

		}
		else//此页已经在内存中，调用之
		{
			//未被使用的时间清零
			pagecontrol[pages[main_t[i]].frame_id].no_use_time = 0;
			continue;
		}
	}
	
	std::cout << "\t命中率：" << (1 - (float)diseffect/(float)total_instruction)*100 << "%\n";
}

/*
 * NUR算法中打印信息
 */
void NUR_print_info()
{

}

//定义周期
const int CLEAR_PERIOD = 5;
/*
 * 模拟NRU算法
 */
void NUR()
{
	std::cout << "NUR: \n";

	clear_memory();

	int period = -1;
	for(int i = 0; i < total_instruction; ++i)
	{
		//判断时间周期是否已经完成
		++period;
		if(period >= CLEAR_PERIOD)//周期以到，清空使用信息
		{
			for(int j = 0; j < AP; ++j)
			{
				pagecontrol[j].has_used = false;
			}
		}


		if(!pages[main_t[i]].is_in_memory)//此页不在内存中。
		{
			++diseffect;
			
			//寻找空闲的帧
			int free_frame_id = -1;
			for(int j = 0; j < AP; ++j)
			{
				if(pagecontrol[j].is_free)
				{
					free_frame_id = j;
					break;
				}
			}

			if(free_frame_id < 0)//没有空闲的帧
			{
				//寻找此周期内未被使用的帧
				//并选择id最小的一个
				int tmp_id = -1;
				for(int j = 0; j < AP; ++j)
				{
					if(!pagecontrol[j].has_used)
					{
						tmp_id = j;
						break;
					}
				}

				if(tmp_id < 0)//没有未被使用的帧，所有帧在此周期内被访问过。
				{
					//置换第一个帧中的页面。
					tmp_id = 0;
				}
				
				//置换出页面
				pages[pagecontrol[tmp_id].page_id].is_in_memory = false;
				
				//更新帧和页面的信息
				pagecontrol[tmp_id].page_id = main_t[i];
				pagecontrol[tmp_id].has_used = true;

				pages[main_t[i]].frame_id = tmp_id;
				pages[main_t[i]].is_in_memory = true;

			}
			else //有空闲的帧
			{
				pagecontrol[free_frame_id].is_free = false;
				pagecontrol[free_frame_id].page_id = main_t[i];
				pagecontrol[free_frame_id].has_used = true;

				pages[main_t[i]].frame_id = free_frame_id;
				pages[main_t[i]].is_in_memory = true;
			}
		}
		else//此页已经在内存中了
		{
			//标记此帧在这个周期中被使用了。
			pagecontrol[main_t[i]].has_used = true;

		}

	}

	std::cout << "\t命中率：" << (1 - (float)diseffect/(float)total_instruction)*100 << "%\n";
}

/*
 *
 * 程序入口
 * 
 */ 
void run()
{
	std::cout << "running...\n";

	//初始化
	init();

	FIFO();
	LRU();
	NUR();
}
