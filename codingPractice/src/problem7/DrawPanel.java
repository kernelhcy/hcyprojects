package problem7;

import java.awt.Color;
import java.awt.Graphics;
import java.awt.Graphics2D;
import java.util.ArrayList;
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
		Shapes = new ArrayList<MyShape>();
		cll = new CreateLineListener(this);
		this.addMouseListener(cll);
		this.addMouseMotionListener(cll);
	}

	public void addSharp(MyShape s)
	{
		Shapes.add(s);

	}

	public MyShape deleteSharp(MyShape s)
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
		
		Color c = null, oldc = null;
		for (MyShape s : Shapes) {
			oldc = g2d.getColor();
			c = s.getColor();
			g2d.setColor(c);
			if(s.isFill()){
				g2d.fill(s.getShape());
			}else{
				g2d.draw(s.getShape());
			}
			g2d.setColor(oldc);
		}
	}
	
	private ArrayList<MyShape> Shapes;
	private CreateLineListener cll;
}
