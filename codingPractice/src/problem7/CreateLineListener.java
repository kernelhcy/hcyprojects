package problem7;

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
		state = 0;
		this.listenee = listenee;
	}

	@Override
	public void mouseDragged(MouseEvent e)
	{
		// TODO Auto-generated method stub
		tmp.x2 = e.getX();
		tmp.y2 = e.getY();
		listenee.repaint();
	}

	@Override
	public void mousePressed(MouseEvent e)
	{
		// TODO Auto-generated method stub
		tmp = new Line2D.Double();
		tmp.x1 = e.getX();
		tmp.y1 = e.getY();
		tmp.x2 = e.getX();
		tmp.y2 = e.getY();
		listenee.addSharp(tmp);
		System.out.println("Create line. Begin at("
				+ tmp.x1 + "," + tmp.y1 + ")");
	}

	@Override
	public void mouseReleased(MouseEvent e)
	{
		// TODO Auto-generated method stub
		tmp.x2 = e.getX();
		tmp.y2 = e.getY();
		listenee.repaint();
		System.out.println("Create line. End at("
					+ tmp.x2 + "," + tmp.y2 + ")");

	}

	/*
	 * 0 : 空闲
	 * 1 : 鼠标press，确定开始的点。此时鼠标在拖动。
	 * 2 : 鼠标松开，确定线。
	 */
	private int state;
	private DrawPanel listenee; // 被监听的对象。
	private Line2D.Double tmp;
}
