package problem10;

import java.awt.BorderLayout;

import javax.swing.JFrame;
import javax.swing.JPanel;

public class MainFrame extends JFrame
{
	public MainFrame()
	{
		this.setTitle("Othello");
		this.setSize(680, 450);
		
		this.setResizable(false);
		
		this.setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);
		ip = new InfoPanel();
		brd = new Board(ip);
		ip.setBoard(brd);
		this.getContentPane().add(brd, BorderLayout.CENTER);
		JPanel p = new JPanel();
		p.add(ip);
		this.getContentPane().add(p, BorderLayout.EAST);
	}
	
	private Board brd;
	private InfoPanel ip;
}
