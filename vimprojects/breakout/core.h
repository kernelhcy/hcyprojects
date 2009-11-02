#ifndef _CORE_H
#define _CORE_H

#include "ball.h"
#include "brick.h"
#include "paddle.h"
#include "config.h"

class Ball;
class Brick;
class Paddle;
class Config;

/*
 * 游戏核心类
 * 定义游戏的规则。
 */
class Core
{
	public:
		Core(Ball *_ball, Brick **_bricks, Paddle *_paddle, int b_cnt);
		~Core();
		
		
		/* 
		     判断球的碰撞 
		   	返回：
		   		1  : 胜利
		   		0  : 发生了碰撞，游戏没有结束
		   		-1 : 失败
		 */
    	int checkCollision();

		/*
		 * 分别设置为随机模式和固定模式
		 */
		void setRandom();
		void noRandom();

		/*
		 * 设置paddle是否可以上下移动。
		 */
		void setPaddleFly(bool);
		bool getPaddleFly();
		
	private:
		Ball *ball;
    	Paddle *paddle;
    	Brick ** bricks; 	/* 盛放brick的数组 */
    	int brick_cnt;
    	
		/*
		 * 设置ball碰到paddle的反射模式
		 *
		 * random_reflect = TRUE  
		 * 			: 随机模式。
		 * 			  ball碰到paddle的五个区域中的一个后，其反射的路径是随机的。
		 * 			  且每次碰撞到同一个区域的反射路径也是随机的，也就是，第一
		 * 			  一次碰撞和以后的碰撞的反射路径也不一定相同。
		 * 						
		 * random_reflect = FALSE 
		 * 			: 固定模式。
		 * 			  ball与paddle碰正之后，其反射路径是固定的。
		 * 			  即，ball与paddle的一个区域碰撞后，其反射的路径固定不变。
		 */
		bool random_reflect;
		/*
		 * xDir和yDir所能取的值对。
		 * 用于随机模式下，随机设置xDir和yDir.
		 */
		int *x_y_dir_s;

		/*
		 * 设置paddle是否可以上下移动。
		 * TRUE  ： 可以。
		 * FALSE ： 不可以。
		 */
		bool paddle_fly;
		/* 
		 * 游戏在随机模式下的速度
		 * 主要是ball移动的速度
		 */
		int speed;
		
		Config *config;
};

#endif
