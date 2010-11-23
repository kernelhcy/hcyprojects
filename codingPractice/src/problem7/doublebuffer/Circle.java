package problem7.doublebuffer;

import java.awt.Color;
import java.awt.Graphics2D;
import java.awt.Image;
import java.awt.Point;

import javax.swing.JPanel;

public class Circle extends Shape
{
	public Point center;
	public int radius;
	
	public Circle()
	{
		center = null;
		radius = -1;
		color = Color.BLACK;
		fill = false;
	}
	
	@Override
	public void draw()
	{
		// TODO Auto-generated method stub
		System.out.println("Not yet implement!");
	}

	@Override
	public void draw(JPanel drawee)
	{
		// TODO Auto-generated method stub
		System.out.println("Not yet implement!");
	}

	@Override
	public void draw(Image image)
	{
		// TODO Auto-generated method stub
		if(image == null || center == null || radius < 0){
			return;
		}
		drawHelp((Graphics2D)image.getGraphics());
	}
	
	private void drawHelp(Graphics2D g)
	{
		if(g == null){
			return;
		}
		
		Color oldc = g.getColor();
		g.setColor(color);
		if(fill){
			g.fillOval(center.x - radius, center.y - radius
					, radius, radius);
		}else{
			g.drawOval(center.x - radius, center.y - radius
					, radius, radius);
		}
		g.setColor(oldc);
		return;
	}

}
