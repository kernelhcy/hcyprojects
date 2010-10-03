package closestPoints;

import java.lang.reflect.InvocationTargetException;

import javax.swing.SwingUtilities;

public class Main
{

	/**
	 * @param args
	 */
	public static void main(String[] args) {
		// TODO Auto-generated method stub
		try {
			SwingUtilities.invokeAndWait(new Runnable()
			{
				
				@Override
				public void run() {
					// TODO Auto-generated method stub
					MainFrame frame = new MainFrame();
					//frame.pack();
					
				}
			});
		}
		catch (InterruptedException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
		catch (InvocationTargetException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
	}

}
