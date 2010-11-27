package problem9;

import java.awt.BorderLayout;
import java.awt.Dimension;
import java.awt.FlowLayout;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;

import javax.swing.JButton;
import javax.swing.JFrame;
import javax.swing.JOptionPane;
import javax.swing.JPanel;

public class MainFrame extends JFrame
{
	public MainFrame()
	{
		setTitle("Eight Queues");
		setDefaultCloseOperation(
				JFrame.EXIT_ON_CLOSE);
		setMinimumSize(
			new Dimension(400, 450));
		getContentPane().setLayout(
				new BorderLayout());
		
		board = new Board();
		getContentPane().add(board, BorderLayout.CENTER);
		
		startBtn = new JButton("Start");
		stopBtn = new JButton("Stop");
		stopBtn.setEnabled(false);
		restartBtn = new JButton("Restart");
		reBtn = new JButton("Get a Result");
		JPanel p = new JPanel();
		FlowLayout fl = new FlowLayout();
		fl.setAlignment(FlowLayout.CENTER);
		p.setLayout(fl);
		p.add(startBtn);
		p.add(stopBtn);
		p.add(restartBtn);
		p.add(reBtn);
		
		reBtn.addActionListener(new ActionListener()
		{
			
			@Override
			public void actionPerformed(ActionEvent e)
			{
				// TODO Auto-generated method stub
				if(!board.getResult()){
					JOptionPane.showMessageDialog(me
						, "Can not put all queues");
				}
				startBtn.setEnabled(true);
				stopBtn.setEnabled(false);
			}
		});
		
		startBtn.addActionListener(new ActionListener()
		{
			
			@Override
			public void actionPerformed(ActionEvent e)
			{
				// TODO Auto-generated method stub
				board.restart();
				startBtn.setEnabled(false);
				stopBtn.setEnabled(true);
			}
		});
		restartBtn.addActionListener(new ActionListener()
		{
			
			@Override
			public void actionPerformed(ActionEvent e)
			{
				// TODO Auto-generated method stub
				board.restart();
				startBtn.setEnabled(false);
				stopBtn.setEnabled(true);
			}
		});
		stopBtn.addActionListener(new ActionListener()
		{
			
			@Override
			public void actionPerformed(ActionEvent e)
			{
				// TODO Auto-generated method stub
				board.stop();
				startBtn.setEnabled(true);
				stopBtn.setEnabled(false);
			}
		});
		getContentPane().add(p
				, BorderLayout.SOUTH);
	}
	
	private JButton reBtn, restartBtn, startBtn, stopBtn;
	private Board board;
	private MainFrame me = this;
}
