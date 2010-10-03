package closestPoints;

import java.awt.Color;

public class PaintPoint
{
	public int x,y;
	public int radius;
	public Color color;
	
	public PaintPoint()
	{
		radius = 7;
		color = Color.BLUE;
	}
	
	public String toString()
	{
		return " (" + x + "," + y + ") ";
	}
}
