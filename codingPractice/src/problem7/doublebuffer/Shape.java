package problem7.doublebuffer;

import java.awt.Image;

import javax.swing.JPanel;

public interface Shape
{
	void draw();

	/**
	 * Directly draw the shape on the panel .
	 * @param drawee
	 */
	void draw(JPanel drawee);
	
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
	void draw(Image image);
}
