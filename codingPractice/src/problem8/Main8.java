package problem8;

import java.lang.reflect.InvocationTargetException;

import javax.swing.SwingUtilities;
import javax.swing.UIManager;
import javax.swing.UnsupportedLookAndFeelException;

public class Main8
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
					UIManager.LookAndFeelInfo[] lafi 
					= UIManager.getInstalledLookAndFeels();
					for(UIManager.LookAndFeelInfo i : lafi){
						System.out.println(i.getClassName());
					}
					
					try {
						UIManager.setLookAndFeel(
						"com.sun.java.swing.plaf.motif.MotifLookAndFeel");
					}
					catch (ClassNotFoundException e) {
						// TODO Auto-generated catch block
						e.printStackTrace();
					}
					catch (InstantiationException e) {
						// TODO Auto-generated catch block
						e.printStackTrace();
					}
					catch (IllegalAccessException e) {
						// TODO Auto-generated catch block
						e.printStackTrace();
					}
					catch (UnsupportedLookAndFeelException e) {
						// TODO Auto-generated catch block
						e.printStackTrace();
					}
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
