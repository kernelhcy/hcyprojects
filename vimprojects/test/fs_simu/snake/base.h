#ifndef __BASE_H_
#define __BASE_H_

#define GREEN_ON_BLACK	1
#define BLUE_ON_BLACK 	2
#define RED_ON_BLACK 	3

#define FOOD_SHARP ACS_DEGREE			//食物的形状
#define SNAKE_INIT_LEN  3 				//蛇的初始长度
#define SNAKE_NODE_SHARP ACS_DIAMOND 	//蛇的形状

#define FOOD_MAX_NUM_PER_LEVEL 30		//每一关的最大食物量

#define MAX_LEVEL 	5 					//最大关数

/**
 * 定义运动的方向
 */
typedef enum _direction
{
	UP_DCT, 	//向上运动
	DOWN_DCT, 	//向下运动
	LEFT_DCT, 	//向左运动
	RIGHT_DCT, 	//向右运动
	UNKNOWN_DCT //未知方向
}dct_t;

//线程的返回值
enum
{
	THD_RETVAL_NORMAL, 		//正常推出
	THD_RETVAL_ERROR, 		//出错

	THD_RETVAL_SNAKE_CRAFT, //蛇的运动违规
	THD_RETVAL_SNAKE_NEXT,  //进入下一关
	
	//键盘事件监听进程的返回值。
	THD_RETVAL_KEY_UP,
	THD_RETVAL_KEY_DWON,
	THD_RETVAL_KEY_LEFT,
	THE_RETVAL_KEY_RIGHT,
	
	THD_RETVAL_UNKNOWN 		//未知类型
};


#endif
