package problem7;

import java.awt.Color;
import java.awt.Polygon;
import java.awt.Shape;
import java.awt.geom.Ellipse2D;
import java.awt.geom.Line2D;
import java.awt.geom.Rectangle2D;
import java.io.Serializable;

/**
 * 自定义形状。
 * 
 * 包含颜色和是否填充。
 * 
 * @author hcy
 * 
 */
public class MyShape implements Serializable
{

	/**
	 * 
	 */
	private static final long serialVersionUID = -6099465306558492256L;
	public MyShape(ShapeType st) 
	{
		this.st = st;
		switch (st) {
		case LINE:
			shape = new Line2D.Double();
			break;
		case ELLI:
			shape = new Ellipse2D.Double();
			break;
		case RECT:
			shape = new Rectangle2D.Double();
			break;
		case POLYGON:
			shape = new Polygon();
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

	/**
	 * 将图形转换为字符串描述。
	 */
	public String toString()
	{
		StringBuffer sb = new StringBuffer("MyShape=");
		switch (st) {
		case LINE:
			sb.append("line ");
			Line2D.Double l2d = (Line2D.Double)shape;
			sb.append("x1=");sb.append(l2d.x1);sb.append(' ');
			sb.append("y1=");sb.append(l2d.y1);sb.append(' ');
			sb.append("x2=");sb.append(l2d.x2);sb.append(' ');
			sb.append("y2=");sb.append(l2d.y2);sb.append(' ');
			sb.append("fill=");sb.append(fill);sb.append(' ');
			sb.append("color=");sb.append(colorToString(c));
			break;
		case ELLI:
			sb.append("elli ");
			Ellipse2D.Double e = (Ellipse2D.Double)shape;
			sb.append("x=");sb.append(e.x);sb.append(' ');
			sb.append("y=");sb.append(e.y);sb.append(' ');
			sb.append("width=");sb.append(e.width);sb.append(' ');
			sb.append("height=");sb.append(e.height);sb.append(' ');
			sb.append("fill=");sb.append(fill);sb.append(' ');
			sb.append("color=");sb.append(colorToString(c));
			break;
		case RECT:
			sb.append("rectangle ");
			Rectangle2D.Double r2d = (Rectangle2D.Double)shape;
			sb.append("x=");sb.append(r2d.x);sb.append(' ');
			sb.append("y=");sb.append(r2d.y);sb.append(' ');
			sb.append("width=");sb.append(r2d.width);sb.append(' ');
			sb.append("height=");sb.append(r2d.height);sb.append(' ');
			sb.append("fill=");sb.append(fill);sb.append(' ');
			sb.append("color=");sb.append(colorToString(c));
			break;
		default:
			return "";
		}
		return sb.toString();
	}
	
	private String colorToString(Color c)
	{
		if(c == null){
			return "";
		}
		StringBuffer sb = new StringBuffer();
		sb.append('(');
		sb.append(c.getRed());
		sb.append(',');
		sb.append(c.getGreen());
		sb.append(',');
		sb.append(c.getBlue());
		sb.append(')');
		return sb.toString();
	}
	
	private boolean fill = false;
	private Color c;
	private Shape shape = null;
	private ShapeType st = ShapeType.NONE;

}
