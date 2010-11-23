package problem7.doublebuffer;

import java.awt.Color;
import java.awt.Point;
import java.lang.reflect.InvocationTargetException;

import javax.swing.JFrame;
import javax.swing.SwingUtilities;

public class Test
{

	/**
	 * @param args
	 * @throws InvocationTargetException
	 * @throws InterruptedException
	 */
	public static void main(String[] args) throws InterruptedException,
			InvocationTargetException
	{
		// TODO Auto-generated method stub
//		try {
//			UIManager.setLookAndFeel(
//			"javax.swing.plaf.metal.MetalLookAndFeel");
//		}
//		catch (Exception e1) {
//			// TODO Auto-generated catch block
//			e1.printStackTrace();
//		}
		
		SwingUtilities.invokeAndWait(new Runnable()
		{

			@Override
			public void run()
			{
				// TODO Auto-generated method stub
				JFrame mf = new JFrame("DB");
				mf.setDefaultCloseOperation(
						JFrame.EXIT_ON_CLOSE);
				
				mf.setSize(800, 700);
				DrawPanelDB dp = new DrawPanelDB();
				mf.getContentPane().add(dp);
				mf.setVisible(true);
				Line l = new Line();
				l.start = new Point(0, 0);
				l.end = new Point(100, 100);
				l.color = Color.RED;
				dp.addShape(l);
				
				Circle c = new Circle();
				c.center = new Point(100, 100);
				c.radius = 50;
				c.color = Color.BLUE;
				c.fill = true;
				dp.addShape(c);
				
				Ellipse e = new Ellipse();
				e.center = new Point(200, 200);
				e.width = 100;
				e.height = 50;
				e.color = Color.RED;
				dp.addShape(e);
				
				Rectangle r = new Rectangle();
				r.width = 100;
				r.height = 30;
				r.x = 500;
				r.y = 30;
				r.fill = true;
				dp.addShape(r);
				
				Polygon p = new Polygon();
				p.fill = true;
				p.color = Color.GREEN;
				p.addPoint(new Point(300, 300));
				p.addPoint(new Point(300, 350));
				p.addPoint(new Point(350, 350));
				p.addPoint(new Point(500, 400));
				p.addPoint(new Point(500, 600));
				dp.addShape(p);
			}
		});
	}

}
