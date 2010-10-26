package knapsack;

import java.awt.BorderLayout;
import java.awt.Container;
import java.awt.FlowLayout;
import java.awt.GridLayout;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.awt.event.FocusAdapter;
import java.awt.event.FocusEvent;
import javax.swing.BorderFactory;
import javax.swing.JButton;
import javax.swing.JFrame;
import javax.swing.JLabel;
import javax.swing.JOptionPane;
import javax.swing.JPanel;
import javax.swing.JScrollPane;
import javax.swing.JTextArea;
import javax.swing.JTextField;
import javax.swing.SwingUtilities;
import javax.swing.text.JTextComponent;

public class KnapsackMain
{

	/**
	 * @param args
	 */
	public static void main(String[] args)
	{
		// TODO Auto-generated method stub
		try {
			SwingUtilities.invokeLater(new Runnable()
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
		}catch (Exception e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
	}

	private static class MainFrame extends JFrame
	{
		public MainFrame() {
			this.setTitle("0-1 Knapsack Problem");
			this.setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);
			this.setSize(800, 500);
			this.setMaximumSize(this.getSize());
			this.setMinimumSize(this.getSize());

			Container con = this.getContentPane();
			con.setLayout(new BorderLayout());
			JPanel p1 = new JPanel();
			con.add(p1, BorderLayout.NORTH);

			JPanel p2 = new JPanel();
			fler = new FocusListener();
			p2.setLayout(new GridLayout(1, 4));
			con.add(p2, BorderLayout.NORTH);
			JLabel nLabel = new JLabel("The number of goods:");
			p2.add(nLabel);
			num = new JTextField("number");
			// num.setCursor(new Cursor(Cursor.TEXT_CURSOR));
			num.addFocusListener(fler);
			p2.add(num);
			JLabel kLabel = new JLabel("The capacity of knapsack:");
			p2.add(kLabel);
			knapsack = new JTextField("capacity");
			// knapsack.setCursor(new Cursor(Cursor.TEXT_CURSOR));
			knapsack.addFocusListener(fler);
			p2.add(knapsack);

			JPanel p3 = new JPanel(new GridLayout(2, 1));
			JPanel p4 = new JPanel(new BorderLayout());
			JPanel p5 = new JPanel(new BorderLayout());
			p3.add(p4);
			p3.add(p5);
			con.add(p3, BorderLayout.CENTER);
			weightArea = new JTextArea();
			JScrollPane wjs = new JScrollPane(weightArea);
			wjs.setBorder(BorderFactory
					.createTitledBorder("Weight:"));
			wjs.setVerticalScrollBarPolicy(
					JScrollPane.VERTICAL_SCROLLBAR_AS_NEEDED);
			wjs.setHorizontalScrollBarPolicy(
					JScrollPane.HORIZONTAL_SCROLLBAR_NEVER);
			valueArea = new JTextArea();
			JScrollPane vjs = new JScrollPane(valueArea);
			vjs.setBorder(BorderFactory
					.createTitledBorder("Values:"));
			vjs.setVerticalScrollBarPolicy(
					JScrollPane.VERTICAL_SCROLLBAR_NEVER);
			vjs.setHorizontalScrollBarPolicy(
					JScrollPane.HORIZONTAL_SCROLLBAR_AS_NEEDED);
			p4.add(wjs, BorderLayout.CENTER);
			p5.add(vjs, BorderLayout.CENTER);

			maxValue = new JLabel();
			runBtn = new JButton("Run...");
			runBtn.addActionListener(new ActionListener()
			{

				@Override
				public void actionPerformed(ActionEvent e)
				{
					// TODO Auto-generated method stub
					int maxV = run();
					if(maxV == -1){
						return;
					}
					//maxValue.setText("Max Value: " + maxV);
					StringBuffer sb = new StringBuffer();
					sb.append("The Max Value is: ");
					sb.append(maxV);
					sb.append("\nThe id of the " +
							"selected goods are:\n");
					int[] re = Knapsack.getResult();
					for(int i = 0;i < re.length; ++i){
						if(re[i] == 1){
							sb.append(i + 1);
							sb.append(' ');
						}
					}
					JOptionPane.showMessageDialog(mf
						, sb.toString());
				}
			});
			clearBtn = new JButton("Clear");
			clearBtn.addActionListener(new ActionListener()
			{

				@Override
				public void actionPerformed(ActionEvent e)
				{
					// TODO Auto-generated method stub
					num.setText("");
					knapsack.setText("");
					weightArea.setText("");
					valueArea.setText("");
					maxValue.setText("");
				}
			});
			exitBtn = new JButton("Exit");
			exitBtn.addActionListener(new ActionListener()
			{

				@Override
				public void actionPerformed(ActionEvent e)
				{
					// TODO Auto-generated method stub
					System.exit(0);
				}
			});
			JPanel p6 = new JPanel(new FlowLayout());
			p6.add(maxValue);
			p6.add(runBtn);
			p6.add(clearBtn);
			p6.add(exitBtn);
			con.add(p6, BorderLayout.SOUTH);

			mf = this;
		}

		private int run()
		{
			int n = 0;
			int pc = 0;
			try {
				n = Integer.valueOf(num.getText());
			}catch (Exception exp) {
				// TODO: handle exception
				showError(num);
				return -1;
			}

			try {
				pc = Integer.valueOf(knapsack.getText());
			}catch (Exception exp) {
				// TODO: handle exception
				showError(num);
				return -1;
			}
			
			int[] weights = new int[n];
			int[] valuess = new int[n];
			
			String[] ws = weightArea.getText().split("\\s");
			String[] vs = valueArea.getText().split("\\s");
			for(int i = 0; i < n; ++i){
				try{
					weights[i] = Integer.valueOf(ws[i]);
				}catch (Exception exp) {
					// TODO: handle exception
					showError(weightArea);
					return -1;
				}
				try{
					valuess[i] = Integer.valueOf(vs[i]);
				}catch (Exception exp) {
					// TODO: handle exception
					showError(valueArea);
					return -1;
				}
			}
			
			int maxV = Knapsack.run(weights, valuess, pc);
			
			return maxV;
		}
		
		/**
		 * 现实错误输入。
		 * @param tc 呈现错误输入的组建。
		 */
		private void showError(JTextComponent tc)
		{
			JOptionPane.showMessageDialog(
					mf,
					"Invalid Input!! " + tc.getText(),
					"ERROR",
					JOptionPane.ERROR_MESSAGE);
			tc.requestFocus();
		}

		private JButton runBtn, clearBtn, exitBtn;
		private JTextField num, knapsack;
		private JTextArea weightArea, valueArea;
		private JLabel maxValue;
		private FocusListener fler;
		private MainFrame mf;
	}

	private static class FocusListener extends FocusAdapter
	{
		public void focusGained(FocusEvent e)
		{
			JTextField tf = (JTextField) e.getSource();
			tf.setSelectionStart(0);
			tf.setSelectionEnd(tf.getText().length());
		}

		public void focusLost(FocusEvent e)
		{
			JTextField tf = (JTextField) e.getSource();
			tf.setSelectionStart(0);
			tf.setSelectionEnd(0);
		}
	}
}
