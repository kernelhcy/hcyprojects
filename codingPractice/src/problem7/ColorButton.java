package problem7;

import java.awt.Color;
import java.awt.Graphics;
import javax.swing.JButton;

/**
 * 颜色选择按钮。
 * 在按钮上显示选择的颜色。
 * @author hcy
 *
 */
public class ColorButton extends JButton
{
	public ColorButton()
	{
		this.setText("     ");
		c = Color.BLACK;
		this.setToolTipText("Select the color");
		
	}
	
	public void paint(Graphics g)
	{
		super.repaint();
		g.setColor(c);
		g.fillRect(5, 5, 20, 20);
		g.setColor(Color.BLACK);
	}
	
	public void setColor(Color c)
	{
		this.c = c;
		this.repaint();
	}
	
	public Color getColor()
	{
		return c;
	}
	
	private Color c;
}
