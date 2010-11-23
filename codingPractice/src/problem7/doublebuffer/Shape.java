package problem7.doublebuffer;

import java.awt.Color;
import java.awt.Image;

import javax.swing.JPanel;

public abstract class Shape
{
	
	public Color color;
	public boolean fill; //fill or draw?
	
	public abstract void draw();

	/**
	 * Directly draw the shape on the panel .
	 * @param drawee
	 */
	public abstract void draw(JPanel drawee);
	
	/**
	 * This is the core function.
	 *  
	 * We just draw the shape on the image, not directly
	 * on the panel or other devices. Then we just draw the
	 * image on the panel or other devices. 
	 * 
	 * Every shape MUST implements this func.
	 * @param image
	 */
	public abstract void draw(Image image);
}
