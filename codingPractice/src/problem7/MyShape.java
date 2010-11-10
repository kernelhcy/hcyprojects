package problem7;

import java.awt.Color;
import java.awt.Shape;
import java.awt.geom.Ellipse2D;
import java.awt.geom.Line2D;
import java.awt.geom.Rectangle2D;

/**
 * 自定义形状。
 * 
 * 包含颜色和是否填充。
 * 
 * @author hcy
 * 
 */
public class MyShape
{

	public MyShape(ShapeType st) 
	{
		switch (st) {
		case LINE:
			shape = new Line2D.Double();
			break;
		case CIRCLE:
			shape = new Ellipse2D.Double();
			break;
		case RECT:
			shape = new Rectangle2D.Double();
			break;
		default:
			throw new IllegalArgumentException(
					"Unkown Shape Type!!");
		}
	}

	public Color getColor()
	{
		// TODO Auto-generated method stub
		return c;
	}

	public Color setColor(Color c)
	{
		// TODO Auto-generated method stub
		Color oldc = this.c;
		this.c = c;
		return oldc;
	}

	public boolean isFill()
	{
		// TODO Auto-generated method stub
		return fill;
	}

	public void setFill(boolean f)
	{
		// TODO Auto-generated method stub
		fill = f;
	}

	public Shape getShape()
	{
		// TODO Auto-generated method stub
		return shape;
	}

	private boolean fill = false;
	private Color c;
	private Shape shape = null;

	private MyMouseAdapter ma;
}
