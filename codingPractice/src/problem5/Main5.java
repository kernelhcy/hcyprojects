package problem5;

import java.lang.reflect.InvocationTargetException;

import javax.swing.SwingUtilities;

public class Main5
{

	/**
	 * @param args
	 */
	public static void main(String[] args)
	{
		// TODO Auto-generated method stub
		
		try {
			SwingUtilities.invokeAndWait(new Runnable()
			{
				
				@Override
				public void run()
				{
					// TODO Auto-generated method stub
					MainFrame mf = new MainFrame();
					mf.setVisible(true);
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
