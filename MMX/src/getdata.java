import java.awt.Graphics;
import java.awt.image.BufferedImage;
import java.awt.image.WritableRaster;
import java.io.File;
import java.io.IOException;

import javax.imageio.ImageIO;
import javax.swing.JFrame;
import javax.swing.JPanel;

public class getdata extends JPanel {

	BufferedImage temp;

	getdata(BufferedImage temp) {
		this.temp = temp;
	}

	static {

		System.loadLibrary("MMX_C");
	}

	public native static int[] calculate_by_MMX(int[] pic1, int[] pic2, int j);

	@Override
	public void paintComponent(Graphics g) {
		g.drawImage(temp, 0, 0, null);
	}

	public static void main(String[] args) {
		// TODO Auto-generated method stub
		JFrame a = new JFrame();
		File f1 = new File(
				"");
		File f2 = new File(
				"");

		try {
			BufferedImage image1 = ImageIO.read(f1);
			WritableRaster raster1 = image1.getRaster();
			int width = image1.getWidth();
			int height = image1.getHeight();
			int[] pixels1 = new int[4 * width * height];
			raster1.getPixels(0, 0, width, height, pixels1);

			BufferedImage image2 = ImageIO.read(f2);
			WritableRaster raster2 = image2.getRaster();
			int[] pixels2 = new int[4 * width * height];
			raster2.getPixels(0, 0, width, height, pixels2);

			BufferedImage image_temp = new BufferedImage(width, height, image2
					.getType());
			WritableRaster raster_temp = image_temp.getRaster();
			int[] pixels_temp = new int[4 * width * height];

			JPanel b = new getdata(image_temp);
			b.setSize(width, height);
			a.setSize(width, height);
			a.add(b);
			a.setVisible(true);
			for (int j = 0; j < 100; j++) {

				pixels_temp = ((getdata)b).calculate_by_MMX(pixels1, pixels2,
						j);

				raster_temp.setPixels(0, 0, width, height, pixels_temp);
				b.repaint();
			}

		} catch (IOException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}

	}

}
