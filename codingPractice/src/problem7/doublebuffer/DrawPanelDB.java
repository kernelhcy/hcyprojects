package problem7.doublebuffer;

import java.awt.Dimension;
import java.awt.Graphics;
import java.awt.Image;
import java.util.ArrayList;

import javax.swing.JPanel;

public class DrawPanelDB extends JPanel
{
	public DrawPanelDB()
	{
		shapes = new ArrayList<Shape>();
		setPreferredSize(new Dimension(300, 300));
		setSize(getPreferredSize());
		buffer = null;
	}

	public void addShape(Shape s)
	{
		shapes.add(s);
		this.drawShapes();
	}

	public Shape delShape(Shape s)
	{
		shapes.remove(s);
		return s;
	}

	public void paint(Graphics g)
	{
		g.drawImage(buffer, 0, 0, null);
	}

	public void drawShapes()
	{
		if (buffer == null
				|| 
				(buffer.getWidth(null) != this.getWidth() 
				&& buffer.getHeight(null) 
					!= this.getHeight())) {
			/**
			 * The return value of createImage may be null 
			 * if the component is not displayable.
			 * So before calling this function, the component MUST
			 * be displayed on the screen or its parent MUST be
			 * displayed!! 
			 */
			buffer = this.createImage(this.getWidth(),
					this.getHeight());
		}
		
		System.out.println("Image width: " + buffer.getWidth(null)
						+ "Height: " + buffer.getHeight(null));
		for (Shape s : shapes) {
			s.draw(buffer);
		}
	}

	private ArrayList<Shape> shapes;
	private Image buffer;
}
