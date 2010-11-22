package problem7.doublebuffer;

import java.awt.Color;
import java.awt.Graphics2D;
import java.awt.Image;
import java.awt.Point;

import javax.swing.JPanel;

public class Line implements Shape
{

	@Override
	public void draw()
	{
		// TODO Auto-generated method stub
		System.out.println("Not yet implement!");
		return;
	}

	@Override
	public void draw(JPanel drawee)
	{
		// TODO Auto-generated method stub
		System.out.println("Not yet implement!");
		return;
	}

	@Override
	public void draw(Image image)
	{
		// TODO Auto-generated method stub
		if(image == null)
		{
			return;
		}
		Graphics2D g = (Graphics2D)image.getGraphics();
		this.drawHelp(g);
	}
	
	/**
	 * Draw the line by g
	 * @param g
	 */
	private void drawHelp(Graphics2D g)
	{
		if(start == null || end == null){
			return;
		}
		Color oldcolor = g.getColor();
		g.setColor(color);
		g.drawLine(start.x, start.y, end.x, end.y);
		g.setColor(oldcolor);
		return;
	}
	
	public Point start = null, end = null;
	public Color color = Color.BLACK;

}
