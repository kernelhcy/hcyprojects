package ClosestPointers;

import javax.swing.JFrame;

public class MainFrame extends JFrame
{
	public MainFrame()
	{
		this.setDefaultCloseOperation(EXIT_ON_CLOSE);
		this.setTitle("Get The Closest Pointers");
		this.setSize(1000, 700);
		this.setVisible(true);
		panel = new PaintPanel();
		this.getContentPane().add(panel);
	}
	
	private PaintPanel panel;
}
