package knapsack;
import javax.swing.JFrame;
import javax.swing.SwingUtilities;

public class KnapsackMain
{

	/**
	 * @param args
	 */
	public static void main(String[] args)
	{
		// TODO Auto-generated method stub
		int[] w = {12, 14, 8, 34, 10};
		int[] v = {12, 15, 12, 30, 20};
		int pc = 50;
		System.out.println("Max Value: " + Knapsack.run(w, v, pc));
		
		int[] re = Knapsack.getResult();
		
		for(int i = 0; i < w.length; ++i){
			if(re[i] == 1){
				System.out.print("(" + w[i]
				                 + ", " + v[i] + ")");
			}
		}
		System.out.println();
		
		try {
			SwingUtilities.invokeAndWait(new Runnable()
			{
				
				@Override
				public void run()
				{
					// TODO Auto-generated method stub
					KnapsackMain.MainFrame mf 
						= new KnapsackMain.MainFrame();
					mf.setVisible(true);
				}
			});
		}
		catch (Exception e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
	}
	
	private static class MainFrame extends JFrame
	{
		public MainFrame()
		{
			this.setTitle("0-1 Knapsack Problem");
			this.setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);			
			this.setSize(500, 500);
			
		}
	}

}
