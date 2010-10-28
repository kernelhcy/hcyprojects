package problem7;

import java.awt.Color;
import java.awt.Dimension;
import java.awt.Graphics2D;
import java.awt.Point;
import java.io.File;

import javax.swing.JComponent;
import javax.swing.JPanel;

public class Line extends Sharp
{

	public Line(Color c, Point begin, Point end)
	{
		this.c = c;
		this.begin = begin;
		this.end = end;
	}
	
	public Line()
	{
		this(Color.BLACK, new Point(0, 0), new Point(1000, 1000));
	}
	
	@Override
	public void paintOn(JComponent panel)
	{
		// TODO Auto-generated method stub
		//设置panel的大小。
		Dimension dim = new Dimension(
				begin.x > end.x ? begin.x : end.x,
				begin.y > end.y ? begin.y : end.y);
		panel.setPreferredSize(dim);
		
		Graphics2D g2d = (Graphics2D)panel.getGraphics();
		Color oldc = g2d.getColor();
		g2d.setColor(c);
		g2d.drawLine(begin.x, begin.y, end.x, end.y);
		g2d.setColor(oldc);
	}

	@Override
	public int writeToFile(File f, int off)
	{
		// TODO Auto-generated method stub
		return 0;
	}

	@Override
	public int readFromFile(File f, int off)
	{
		// TODO Auto-generated method stub
		return 0;
	}

	@Override
	public void setColor(Color c)
	{
		// TODO Auto-generated method stub

	}

	@Override
	public Color getColor()
	{
		// TODO Auto-generated method stub
		return null;
	}

	@Override
	public void setFill(boolean isFill)
	{
		// TODO Auto-generated method stub

	}

	@Override
	public boolean isFill()
	{
		// TODO Auto-generated method stub
		return false;
	}
	
	private Color c;
	private Point begin, end;
}
