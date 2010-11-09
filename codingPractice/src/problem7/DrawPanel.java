package problem7;

import java.awt.Color;
import java.awt.Graphics;
import java.awt.Graphics2D;
import java.util.ArrayList;
import java.awt.Shape;
import javax.swing.JPanel;
/**
 * 画图面板。
 * 
 * JDK中提供了各种图形的封装类。所有类都实现了Shape接口，
 * 在面板中画这些图形，只需要g2d.draw(s)即可。
 * @author hcy
 *
 */
public class DrawPanel extends JPanel
{
	public DrawPanel() {
		Shapes = new ArrayList<Shape>();
		cll = new CreateLineListener(this);
		this.addMouseListener(cll);
		this.addMouseMotionListener(cll);
	}

	public void addSharp(Shape s)
	{
		Shapes.add(s);

	}

	public Shape deleteSharp(Shape s)
	{
		Shapes.remove(s);
		return s;
	}

	public void paint(Graphics g)
	{
		Graphics2D g2d = (Graphics2D)g;
		//clear
		g2d.setColor(Color.WHITE);
		g2d.fillRect(0, 0, this.getWidth() + 1, this.getHeight() + 1);
		g2d.setColor(Color.BLACK);
		for (Shape s : Shapes) {
			g2d.draw(s);
		}
	}
	
	private ArrayList<Shape> Shapes;
	private CreateLineListener cll;
}
