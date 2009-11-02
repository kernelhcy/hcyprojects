#ifndef BALL_H
#define BALL_H

#include <QImage>
#include <QRect>
/*
 * 球类
 * 模拟撞击的球
 */
class Ball
{

  public:
    Ball();
    ~Ball();

  public:
    void resetState();
	/*
	 * 移动球的方法
	 */
    void moveBottom(int);
    void moveTop(int);
    void moveLeft(int);
    void moveRight(int);

    void autoMove();
    
	void setXDir(int);
    void setYDir(int);
    int getXDir();
    int getYDir();
    
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

	QRect getRect();
    QImage & getImage();

  private:
    int angle;
    int speed;
    int xdir;
    int ydir;
    bool stuck;
    QImage image;
    QRect rect;
	/* 球的位置坐标 */
	int x;
	int y;
	
	/* 球可以移动到的最大坐标位置 */
	int max_x;
	int max_y;

	/* 球可以移动到的最小坐标位置 */
	int min_x;
	int min_y;

};

#endif
