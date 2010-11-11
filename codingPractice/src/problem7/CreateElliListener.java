package problem7;

import java.awt.Color;
import java.awt.event.MouseEvent;
import java.awt.geom.Ellipse2D;
import java.awt.geom.Point2D;

public class CreateElliListener extends MyMouseAdapter
{

	public CreateElliListener(DrawPanel listenee) {
		// TODO Auto-generated constructor stub
		this.listenee = listenee;
		startP = new Point2D.Double();
		endP = new Point2D.Double();
	}
	@Override
	public void mouseDragged(MouseEvent e)
	{
		// TODO Auto-generated method stub
		endP.setLocation(e.getX(), e.getY());
		modifyElli();
		listenee.repaint();
	}

	@Override
	public void mousePressed(MouseEvent e)
	{
		// TODO Auto-generated method stub
		tmp = new MyShape(ShapeType.ELLI);
		tmp.setColor(c);
		tmp.setFill(fill);
		startP.setLocation(e.getX(), e.getY());
		endP.setLocation(startP);
		modifyElli();
		listenee.addShape(tmp);
	}

	@Override
	public void mouseReleased(MouseEvent e)
	{
		// TODO Auto-generated method stub
		endP.setLocation(e.getX(), e.getY());
		modifyElli();
		listenee.repaint();

	}
	
	/**
	 * 根据startP和endP的值对矩形进行修改。
	 * 以适应各种情况。
	 */
	protected void modifyElli()
	{
		if(startP.getX() < endP.getX()){
			/*
			 *  * startP
			 *          
			 *          * endP
			 */
			if(startP.getY() < endP.getY()){
				((Ellipse2D.Double)tmp.getShape()).x
						= startP.getX();
				((Ellipse2D.Double)tmp.getShape()).y
						= startP.getY();
				((Ellipse2D.Double)tmp.getShape()).width
						= endP.getX() - startP.getX();
				((Ellipse2D.Double)tmp.getShape()).height
						= endP.getY() - startP.getY();
			}
			/*
			 *       *endP
			 * 
			 * * startP
			 */
			else{
				((Ellipse2D.Double)tmp.getShape()).x 
					= startP.getX();
				((Ellipse2D.Double)tmp.getShape()).y
					= endP.getY();
				((Ellipse2D.Double)tmp.getShape()).width
					= endP.getX() - startP.getX();
				((Ellipse2D.Double)tmp.getShape()).height
					= startP.getY() - endP.getY();
			}
		}else{
			/*
			 *          * startP
			 *          
			 *  * endP
			 */
			if(startP.getY() < endP.getY()){
				((Ellipse2D.Double)tmp.getShape()).x 
					= endP.getX();
				((Ellipse2D.Double)tmp.getShape()).y
					= startP.getY();
				((Ellipse2D.Double)tmp.getShape()).width
					= startP.getX() - endP.getX();
				((Ellipse2D.Double)tmp.getShape()).height
					= endP.getY() - startP.getY();
			}
			/*
			 * *endP
			 * 
			 *         * startP
			 */
			else{
				((Ellipse2D.Double)tmp.getShape()).x 
					= endP.getX();
				((Ellipse2D.Double)tmp.getShape()).y
					= endP.getY();
				((Ellipse2D.Double)tmp.getShape()).width
					= startP.getX() - endP.getX();
				((Ellipse2D.Double)tmp.getShape()).height
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
