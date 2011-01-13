package problem10;

import java.awt.GridLayout;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;

import javax.swing.JButton;
import javax.swing.JLabel;
import javax.swing.JPanel;

/*
 * 信息面板
 */
public class InfoPanel extends JPanel implements ActionListener
{
	public InfoPanel()
	{
		blackL = new JLabel("Black:");
		whiteL = new JLabel("White:");
		blackNum = new JLabel("2");
		whiteNum = new JLabel("2");
		startBtn = new JButton("Start");
		restartBtn = new JButton("Resart");
		startBtn.addActionListener(this);
		restartBtn.addActionListener(this);
		
		playerL = new JLabel("Current Player: ");
		player = new JLabel("Black");
		
		GridLayout gl = new GridLayout(4, 2);
		gl.setHgap(10);
		gl.setVgap(10);
		
		this.setLayout(gl);
		this.add(blackL);
		this.add(blackNum);
		this.add(whiteL);
		this.add(whiteNum);
		
		this.add(startBtn);
		this.add(restartBtn);
		
		this.add(playerL);
		this.add(player);
	}
	
	public void setBoard(Board b)
	{
		this.brd = b;
	}
	
	@Override
	public void actionPerformed(ActionEvent e)
	{
		// TODO Auto-generated method stub
		JButton src = (JButton)e.getSource();
		if(src == startBtn){
			brd.start();
			blackNum.setText("2");
			whiteNum.setText("2");
		}
		if(src == restartBtn){
			brd.restart();
		}
	} 
	
	private JLabel blackL, whiteL, playerL;
	public JLabel blackNum, whiteNum, player;
	
	private JButton startBtn, restartBtn;
	
	private Board brd;

	
}
