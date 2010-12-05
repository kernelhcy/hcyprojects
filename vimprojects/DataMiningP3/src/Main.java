import javax.swing.*;

public class Main
{
	public static void main(String[] arg)
	{
		SwingUtilities.invokeLater(new Runnable()
		{
			public void run(){
				MainFrame mf = new MainFrame();
				mf.setVisible(true);
			}
		});
	}
}
