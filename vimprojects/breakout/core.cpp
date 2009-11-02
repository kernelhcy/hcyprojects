#include "core.h"

Core::Core(Ball *_ball, Brick **_bricks, Paddle *_paddle, int b_cnt)
			: ball(_ball), bricks(_bricks), paddle(_paddle), brick_cnt(b_cnt)
{
	
	config = Config::getInstance();
	
	if (config -> mode == 1)
	{
		setRandom();
	}
	else
	{
		noRandom();
	}
	
	x_y_dir_s = new int[10];
	/* 1 */
	x_y_dir_s[0] = -1;
	x_y_dir_s[1] = -1;

	/* 2 */
	x_y_dir_s[2] = -1;
	x_y_dir_s[3] = 0;
	
	/* 3 */
	x_y_dir_s[4] = 0;
	x_y_dir_s[5] = -1;
	
	/* 4 */
	x_y_dir_s[6] = 1;
	x_y_dir_s[7] = 0;
	
	/* 5 */
	x_y_dir_s[8] = 1;
	x_y_dir_s[9] = -1;
		
	/* 默认paddle不能上下移动 */
	paddle_fly = FALSE;

	speed = 1;


}
Core::~Core()
{
	delete x_y_dir_s;
}

/*
 * 游戏的核心规则。
 * 程序的核心算法。
 */
int Core::checkCollision()
{

	/* ball出了下界，游戏失败，结束 */
  	if (ball->getRect().bottom() > ball -> getMaxY())
	{
    	return -1;
	}

	/* 判断已经消掉的砖块的数量 */
  	for (int i = 0, j = 0; i < brick_cnt; i++) 
	{
    	if (bricks[i]->isDestroyed()) 
		{
      		j++;
    	}
    	
		/* 所有砖块都消掉，游戏胜利，结束 */
		if (j == brick_cnt)
		{
      		return 1;
		}
  	}
	 
	/* 判断ball和paddle的碰撞情况 */
  	if ((ball->getRect()).intersects(paddle->getRect())) 
	{
		/* 分别获得ball和paddle的左边界的坐标值 */
    	int paddleLPos = paddle->getRect().left();  
    	int ballLPos = ball->getRect().left();   
		/*
		 * 设置五个区域。
		 * 当ball和paddle发生碰撞时，ball的左边界在不同的区域中，ball的反射路线就不一样。
		 * 图示如下：
		 *
		 *    ##  ball
		 *    
		 *      \           \       /        |     \        /           /
		 *       \           \     /         |      \      /           /
		 *        \           \   /          |       \    /           /
		 *         \           \ /           |        \  /           /
		 * 		-----------------------------------------------------------
		 * 		|     1    |     2     |     3     |     4     |     5    |   paddle
		 * 		-----------------------------------------------------------
		 * paddleLPos     +8          +16         +24         +32        
		 *               first       second      third       fourth
		 *
		 * 		上图表示paddle，标注的五个区域的位置。
		 * 		当ball的左边界在区域1时，ball向左上方反射，x和y的速度相同，xDir=yDir=-1;
		 * 		当ball的左边界在区域2时，ball向左上方反射，x的速度为-1,y的速度为-yDir;
		 * 		....................3..，ball向正上方反射，x不变，y的速度为yDir=-1;
		 * 		....................4..，......右上方....，x的速度为1,y的速度为-yDir;
		 * 		....................5..，......右上方....，x和y的速度分别为，xDir=1,yDir=-1;
		 */
    	int first = paddleLPos + 8;
    	int second = paddleLPos + 16;
    	int third = paddleLPos + 24;
    	int fourth = paddleLPos + 32;

		int x_dir;
		int y_dir;
		/* 根据不同的区域设置反射的方向和速度 */
		
		if (random_reflect) 		/*随机模式 */
		{
			/* 设置随机的速度 */
			speed = rand() % 3 + 1;
			
			int r = rand() % 5;
			x_dir = x_y_dir_s[r * 2] * speed;

			if (r == 1 || r == 3)
			{
				y_dir = ball -> getYDir() * - 1 * speed;
			}
			else 
			{
				y_dir = x_y_dir_s[r * 2 + 1] * speed;
			}
		}
		else 						/* 固定模式 */
		{
    		if (ballLPos < first) 
			{
				x_dir = -1;
				y_dir = -1;
    		}

    		if (ballLPos >= first && ballLPos < second) 
			{
				x_dir = -1;
				y_dir = -1 * ball -> getYDir();
    		}

    		if (ballLPos >= second && ballLPos < third) 
			{
				x_dir = 0;
				y_dir = -1;
    		}

    		if (ballLPos >= third && ballLPos < fourth) 
			{
				x_dir = 1;
				y_dir = -1 * ball -> getYDir();
    		}

    		if (ballLPos > fourth) 
			{
				x_dir = 1;
				y_dir = -1;
    		}
		}

    	ball->setXDir(x_dir);
      	ball->setYDir(y_dir);


  	}

	/* 判断ball和每个砖块的碰撞情况 */ 
  	for (int i = 0; i < brick_cnt; i++) 
	{
    	if ((ball->getRect()).intersects(bricks[i]->getRect())) 
		{
			/*
			 *  ball的左上顶点的坐标和ball的长宽
			 */
      		int ballLeft = ball->getRect().left();  
      		int ballHeight = ball->getRect().height(); 
      		int ballWidth = ball->getRect().width();
      		int ballTop = ball->getRect().top();  

			/*
			 * 定义四个点，分别位于ball的图示位置：
			 *
			 *               *pointTop
			 *    pointLeft* ---------- *pointRight
			 *               |        |
			 *               |  ball  |
			 *               |        |
			 *               ---------- 
			 *               *pointBottom
			 *
			 * 根据各个砖块是否包含这四个点中的一个或几个判断ball是否
			 * 于砖块碰撞。
			 * Left: 	x的速度设置为1;
			 * Right: 	x的速度设置为-1;
			 * Top: 	y的速度设置为1;
			 * Bottom: 	y的速度设置为-1;
			 *
			 */
      		QPoint pointRight(ballLeft + ballWidth + 1, ballTop);
      		QPoint pointLeft(ballLeft - 1, ballTop);  
      		QPoint pointTop(ballLeft, ballTop -1);
      		QPoint pointBottom(ballLeft, ballTop + ballHeight + 1);  
		
      		if (!bricks[i]->isDestroyed()) 
			{
				if (random_reflect)
				{
					speed = rand() % 3 + 1;
				}
				else
				{
					speed = 1;
				}
        		if(bricks[i]->getRect().contains(pointRight)) 
				{
           			ball->setXDir(-1 * speed);
        		} 
        		else if(bricks[i]->getRect().contains(pointLeft)) 
				{
           			ball->setXDir(1 * speed);
        		} 

        		if(bricks[i]->getRect().contains(pointTop)) 
				{
           			ball->setYDir(1 * speed);
        		} 
        		else if(bricks[i]->getRect().contains(pointBottom)) 
				{
           			ball->setYDir(-1 * speed);
        		} 

				/* 砖块被撞掉。。。 */
        		bricks[i]->setDestroyed(TRUE);
      		}
    	}
  	}
	
	return 0;
}

void Core::setRandom()
{
	random_reflect = TRUE;
}

void Core::noRandom()
{
	random_reflect = FALSE;
}

void Core::setPaddleFly(bool fly)
{
	paddle_fly = fly;
}
bool Core::getPaddleFly()
{
	return paddle_fly;
}



