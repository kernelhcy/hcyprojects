#ifndef PADDLE_H
#define PADDLE_H

#include <QImage>
#include <QRect>

class Paddle
{

	public:
    	Paddle();
   	 	~Paddle();

  	public:
    	void resetState();
    	void moveLeft(int);
    	void moveRight(int);
    	void moveTop(int);
    	void moveBottom(int);
    	QRect getRect();
    	QImage & getImage();
	
		void setX(int);
		void setY(int);
		int getX();
		int getY();
	
		void setMaxX(int);
		void setMaxY(int);
		int getMaxX();
		int getMaxY();
		
		void setMinX(int);
		void setMinY(int);
		int getMinX();
		int getMinY();


  	private:
    	QImage image;
    	QRect rect;
		/* 位置坐标 */
		int x;
		int y;
	
		/* 所能达到的最大坐标位置 */
		int max_x;
		int max_y;
		/* 所能达到的最小坐标位置 */
		int min_x;
		int min_y;

};

#endif
