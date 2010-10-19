package lis;

import java.awt.BorderLayout;
import java.awt.Dimension;
import java.awt.FlowLayout;
import java.awt.Font;
import java.awt.GridLayout;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.util.Random;

import javax.swing.BorderFactory;
import javax.swing.JButton;
import javax.swing.JFrame;
import javax.swing.JLabel;
import javax.swing.JOptionPane;
import javax.swing.JPanel;
import javax.swing.JScrollPane;
import javax.swing.JTextArea;

public class Main
{

	/**
	 * @param args
	 */
	public static void main(String[] args)
	{
		MainWindow mw =new MainWindow();
		mw.setVisible(true);
	}
	
	private static class MainWindow extends JFrame
	{
		public MainWindow()
		{
			setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);
			setTitle("LIS");
			setSize(700, 500);
			getContentPane().setLayout(new BorderLayout());
			
			runbtn = new JButton("Run!");
			runbtn.addActionListener(new ActionListener()
			{
				
				@Override
				public void actionPerformed(ActionEvent e)
				{
					// TODO Auto-generated method stub
					String ins = input.getText();
					char[] re = new char[ins.length()];
					int len = LIS.getLISLen(
							ins.toCharArray(), re);
					StringBuffer rs = new StringBuffer();
					for(int i = 0; i < len; ++i){
						rs.append(re[i]);
					}
					result.setText(rs.toString());
					lenLabel.setText("length: " + len);
				}
			});
			
			clearbtn = new JButton("Clear");
			clearbtn.addActionListener(new ActionListener()
			{
				
				@Override
				public void actionPerformed(ActionEvent e)
				{
					// TODO Auto-generated method stub
					input.setText("");
					result.setText("");
				}
			});
			
			exitbtn = new JButton("Exit...");
			exitbtn.addActionListener(new ActionListener()
			{
				
				@Override
				public void actionPerformed(ActionEvent e)
				{
					// TODO Auto-generated method stub
					mw.dispose();
				}
			});
			
			randomchars = new JButton("Get random chars");
			randomchars.addActionListener(new ActionListener()
			{
				
				@Override
				public void actionPerformed(ActionEvent e)
				{
					// TODO Auto-generated method stub
					input.setText("");
					result.setText("");
					int n;
					String len = 
						JOptionPane.showInputDialog(mw,
						"Please input the length:"
						, "Input Length"
						, JOptionPane.OK_OPTION);
					if(len == null){
						len = "0";
					}
					n = Integer.valueOf(len);
					if(n > 100000){
						n = 100000;
					}
					StringBuffer sb =new StringBuffer();
					Random rd = new Random();
					int tmp;
					for(int i = 0; i < n; ++i){
						tmp = rd.nextInt(26);
						sb.append((char)('a' + tmp));
					}
					input.setText(sb.toString());
				}
			});
			
			lenLabel = new JLabel();
			lenLabel.setPreferredSize(new Dimension(
					"length: XXXXX".length() * 10
					, runbtn.getPreferredSize().height));
			lenLabel.setFont(new Font(fontname, Font.PLAIN, 20));
			
			input = new JTextArea();
			input.setLineWrap(true);
			input.setFont(new Font(fontname, Font.PLAIN, 20));
			JScrollPane jspInput = new JScrollPane(input);
			jspInput.setHorizontalScrollBarPolicy(
					JScrollPane.HORIZONTAL_SCROLLBAR_NEVER);
			jspInput.setVerticalScrollBarPolicy(
					JScrollPane.VERTICAL_SCROLLBAR_AS_NEEDED);
			jspInput.setBorder(BorderFactory.createTitledBorder(
					"input:"));
			
			result = new JTextArea();
			result.setEditable(false);
			result.setFont(new Font(fontname, Font.PLAIN, 20));
			JScrollPane jspResult = new JScrollPane(result);
			jspResult.setHorizontalScrollBarPolicy(
					JScrollPane.HORIZONTAL_SCROLLBAR_NEVER);
			jspResult.setVerticalScrollBarPolicy(
					JScrollPane.VERTICAL_SCROLLBAR_AS_NEEDED);
			jspResult.setBorder(BorderFactory.createTitledBorder(
					"Result:"));
			
			JPanel jp2 = new JPanel();
			GridLayout glb = new GridLayout(2, 1);
			glb.setVgap(20);
			jp2.setLayout(glb);
			jp2.add(jspInput);
			jp2.add(jspResult);
			
			JPanel jp1 = new JPanel();
			FlowLayout fl = new FlowLayout();
			fl.setAlignment(FlowLayout.CENTER);
			fl.setHgap(10);
			jp1.setLayout(fl);
			jp1.add(lenLabel);
			jp1.add(randomchars);
			jp1.add(runbtn);
			jp1.add(clearbtn);
			jp1.add(exitbtn);
			
			getContentPane().add(jp2, BorderLayout.CENTER);
			getContentPane().add(jp1, BorderLayout.SOUTH);
			mw = this;
		}
		
		private JButton runbtn, clearbtn, exitbtn, randomchars;
		private JTextArea input;
		private JTextArea result;
		private JLabel lenLabel;
		private String fontname = "Courier New";
		private MainWindow mw;
	}

}
