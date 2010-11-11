package problem7;

import javax.swing.SwingUtilities;

public class Main7
{

	/**
	 * @param args
	 */
	public static void main(String[] args)
	{
		// TODO Auto-generated method stub
		SwingUtilities.invokeLater(new Runnable()
		{
			
			@Override
			public void run()
			{
				// TODO Auto-generated method stub
				MainFrame mf = new MainFrame();
				mf.setSize(900, 600);
				mf.setVisible(true);
				
			}
		});
	}

}
