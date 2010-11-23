package problem7.doublebuffer;

import java.awt.Color;
import java.awt.Graphics2D;
import java.awt.Image;
import java.awt.Point;

import javax.swing.JPanel;

public class Ellipse extends Shape
{
	public int width, height;
	public Point center;

	public Ellipse()
	{
		width = height = -1;
		center = null;
		fill = false;
		color = Color.BLACK;
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
		if(image == null || center == null
				|| width < 0 || height < 0){
			return;
		}
		this.drawHelp((Graphics2D)image.getGraphics());
	}
	
	private void drawHelp(Graphics2D g)
	{
		if(g == null){
			return;
		}
		Color oldc = g.getColor();
		g.setColor(color);
		if(fill){
			g.fillOval(center.x - width, center.y - height
					, width, height);
		}else{
			g.drawOval(center.x - width, center.y - height
					, width, height);
		}
		g.setColor(oldc);
	}

}
