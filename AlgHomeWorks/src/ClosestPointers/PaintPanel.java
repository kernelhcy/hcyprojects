package ClosestPointers;

import java.awt.Graphics;
import java.awt.Graphics2D;

import javax.swing.JPanel;

public class PaintPanel extends JPanel
{
	public void paintComponent(Graphics g)
	{
		g.drawLine(100, 100, 100, 100);
		g.drawOval(200, 200, 50, 50);
		Graphics2D g2d = (Graphics2D)g;
		g2d.fillOval(300, 300, 50, 50);
	}
}
