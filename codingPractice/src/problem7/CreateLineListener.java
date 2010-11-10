package problem7;

import java.awt.Color;
import java.awt.event.MouseEvent;
import java.awt.geom.Line2D;

/**
 * 用于画直线。
 * 
 * @author hcy
 * 
 */
public class CreateLineListener extends MyMouseAdapter
{

	public CreateLineListener(DrawPanel listenee) {
		this.listenee = listenee;
		
	}

	@Override
	public void mouseDragged(MouseEvent e)
	{
		// TODO Auto-generated method stub
		((Line2D.Double)tmp.getShape()).x2 = e.getX();
		((Line2D.Double)tmp.getShape()).y2 = e.getY();
		listenee.repaint();
	}

	@Override
	public void mousePressed(MouseEvent e)
	{
		// TODO Auto-generated method stub
		tmp = new MyShape(ShapeType.LINE);
		tmp.setColor(c);
		tmp.setFill(fill);
		((Line2D.Double)tmp.getShape()).x1 = e.getX();
		((Line2D.Double)tmp.getShape()).y1 = e.getY();
		((Line2D.Double)tmp.getShape()).x2 = e.getX();
		((Line2D.Double)tmp.getShape()).y2 = e.getY();
		listenee.addSharp(tmp);
		System.out.println("Create line. Begin at("
				+ ((Line2D.Double)tmp.getShape()).x1 
				+ "," 
				+ ((Line2D.Double)tmp.getShape()).y1 
				+ ")");
	}

	@Override
	public void mouseReleased(MouseEvent e)
	{
		// TODO Auto-generated method stub
		((Line2D.Double)tmp.getShape()).x2 = e.getX();
		((Line2D.Double)tmp.getShape()).y2 = e.getY();
		listenee.repaint();
		System.out.println("Create line. End at("
					+ ((Line2D.Double)tmp.getShape()).x2 
					+ "," 
					+ ((Line2D.Double)tmp.getShape()).y2 
					+ ")");

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
	
	private boolean fill;
	private Color c;
}
