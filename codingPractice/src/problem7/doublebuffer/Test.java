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

		SwingUtilities.invokeAndWait(new Runnable()
		{

			@Override
			public void run()
			{
				// TODO Auto-generated method stub
				JFrame mf = new JFrame("DB");
				mf.setSize(300, 300);
				DrawPanelDB dp = new DrawPanelDB();
				mf.getContentPane().add(dp);
				mf.setVisible(true);
				Line l = new Line();
				l.start = new Point(0, 0);
				l.end = new Point(100, 100);
				l.color = Color.RED;
				dp.addShape(l);
				
			}
		});
	}

}
