package problem7.doublebuffer;

import java.awt.Color;
import java.awt.Graphics2D;
import java.awt.Image;
import java.awt.Point;
import java.util.Arrays;

import javax.swing.JPanel;

public class Polygon extends Shape
{

	public Polygon()
	{
		size = 4;
		xPoints = new int[size];
		yPoints = new int[size];
		nPoints = 0;
	}
	
	public void addPoint(Point p)
	{
		if(nPoints == xPoints.length){
			if(!extend()){
				System.out.println("Extend ERROR!"
					+ "Cannot add the new point.");
				return;
			}
		}
		xPoints[nPoints] = p.x;
		yPoints[nPoints] = p.y;
		++nPoints;
	}
	
	public Point delPoint(Point p)
	{
		int index = -1;
		for(int i = 0; i < nPoints; ++i){
			//only remove the first point that is found
			if(xPoints[i] == p.x
				&& yPoints[i] == p.y){
				index = i;
				break;
			}
		}
		if(index != -1){
			System.arraycopy(xPoints, index + 1
					, xPoints, index, nPoints - index - 1);
			System.arraycopy(yPoints, index + 1
					, yPoints, index, nPoints - index - 1);
			--nPoints;
		}
		return p;
	}
	
	public Point delPoint(int x, int y)
	{
		Point p = new Point(x, y);
		delPoint(p);
		return p;
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
		if(image == null || nPoints <= 0){
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
			g.fillPolygon(xPoints, yPoints, nPoints);
		}else{
			g.drawPolygon(xPoints, yPoints, nPoints);
		}
		g.setColor(oldc);
	}
	
	private boolean extend()
	{
		size *= 2;
		xPoints = Arrays.copyOf(xPoints, size);
		yPoints = Arrays.copyOf(yPoints, size);
		if(xPoints == null || yPoints == null
			|| xPoints.length < size || yPoints.length < size){
			//copy error.
			return false;
		}
		return true;
	}
	
	private int[] xPoints;
	private int[] yPoints;
	private int size;
	private int nPoints;
}
