package problem7;

import java.awt.Color;
import java.awt.event.MouseEvent;
import java.awt.geom.Point2D;
import java.awt.geom.Rectangle2D;

public class CreateRectListener extends MyMouseAdapter
{
	public CreateRectListener(DrawPanel listenee)
	{
		this.listenee = listenee;
		startP = new Point2D.Double();
		endP = new Point2D.Double();
	}
	@Override
	public void mouseDragged(MouseEvent e)
	{
		// TODO Auto-generated method stub
		endP.setLocation(e.getX(), e.getY());
		modifyRect();
		listenee.repaint();
	}

	@Override
	public void mousePressed(MouseEvent e)
	{
		// TODO Auto-generated method stub
		tmp = new MyShape(ShapeType.RECT);
		tmp.setColor(c);
		tmp.setFill(fill);
		startP.setLocation(e.getX(), e.getY());
		endP.setLocation(startP);
		modifyRect();
		listenee.addSharp(tmp);
	}

	@Override
	public void mouseReleased(MouseEvent e)
	{
		// TODO Auto-generated method stub
		endP.setLocation(e.getX(), e.getY());
		modifyRect();
		listenee.repaint();

	}
	
	/**
	 * 根据startP和endP的值对矩形进行修改。
	 * 以适应各种情况。
	 */
	private void modifyRect()
	{
		if(startP.getX() < endP.getX()){
			/*
			 *  * startP
			 *          
			 *          * endP
			 */
			if(startP.getY() < endP.getY()){
				((Rectangle2D.Double)tmp.getShape()).x 
						= startP.getX();
				((Rectangle2D.Double)tmp.getShape()).y
						= startP.getY();
				((Rectangle2D.Double)tmp.getShape()).width
						= endP.getX() - startP.getX();
				((Rectangle2D.Double)tmp.getShape()).height
						= endP.getY() - startP.getY();
			}
			/*
			 *       *endP
			 * 
			 * * startP
			 */
			else{
				((Rectangle2D.Double)tmp.getShape()).x 
					= startP.getX();
				((Rectangle2D.Double)tmp.getShape()).y
					= endP.getY();
				((Rectangle2D.Double)tmp.getShape()).width
					= endP.getX() - startP.getX();
				((Rectangle2D.Double)tmp.getShape()).height
					= startP.getY() - endP.getY();
			}
		}else{
			/*
			 *          * startP
			 *          
			 *  * endP
			 */
			if(startP.getY() < endP.getY()){
				((Rectangle2D.Double)tmp.getShape()).x 
					= endP.getX();
				((Rectangle2D.Double)tmp.getShape()).y
					= startP.getY();
				((Rectangle2D.Double)tmp.getShape()).width
					= startP.getX() - endP.getX();
				((Rectangle2D.Double)tmp.getShape()).height
					= endP.getY() - startP.getY();
			}
			/*
			 * *endP
			 * 
			 *         * startP
			 */
			else{
				((Rectangle2D.Double)tmp.getShape()).x 
					= endP.getX();
				((Rectangle2D.Double)tmp.getShape()).y
					= endP.getY();
				((Rectangle2D.Double)tmp.getShape()).width
					= startP.getX() - endP.getX();
				((Rectangle2D.Double)tmp.getShape()).height
					= startP.getY() - endP.getY();
			}
		}
	}
	
	
	public void setColor(Color c)
	{
		this.c = c;
	}
	
	public void setFill(boolean fill)
	{
		this.fill = fill;
	}
	
	private DrawPanel listenee; // 被监听的对象。
	private MyShape tmp;
	//startP鼠标开始的位置。
	//endp鼠标当前位置。
	private Point2D startP, endP;
	
	private boolean fill;
	private Color c;
}
