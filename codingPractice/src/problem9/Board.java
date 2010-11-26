package problem9;

import java.awt.Color;
import java.awt.Dimension;
import java.awt.Graphics;
import java.awt.Graphics2D;
import java.awt.Image;
import java.awt.event.MouseAdapter;
import java.awt.event.MouseEvent;
import java.util.Calendar;
import java.util.Date;
import java.util.Timer;
import java.util.TimerTask;

import javax.swing.JOptionPane;
import javax.swing.JPanel;
import javax.swing.SwingUtilities;

public class Board extends JPanel
{
	public Board() {
		setPreferredSize(new Dimension(500, 550));
		this.addMouseListener(new MyMouseListener());
		
		calculateSize();
		restart();
	}

	//restart game
	public void restart()
	{
		vx = vy = -1;
		cnt = 0;
		for(int i = 0; i < 8; ++i){
			for(int j = 0; j < 8; ++j){
				board[i][j] = false;
				color[i][j] = Color.BLACK;
			}
		}
		TimerTask task = new TimerTask()
		{
			
			@Override
			public void run()
			{
				// TODO Auto-generated method stub
				SwingUtilities.invokeLater(new Runnable()
				{
					
					@Override
					public void run()
					{
						// TODO Auto-generated 
						//method stub
						Date now = Calendar
							.getInstance()
							.getTime();
						System.out.println(now);
					}
				});
			}
		};
		timer.schedule(task, Calendar.getInstance().getTime()
				,  500);
	}
	
	public void paint(Graphics g)
	{
		if(buffer == null ||
			(buffer.getWidth(null) != this.getWidth()
			&& buffer.getHeight(null) != this.getHeight())){
			buffer = this.createImage(this.getWidth()
					, this.getHeight());
			drawBuffer();
		}
		g.drawImage(buffer, 0, 0, null);
	}
	
	/**
	 * Create a result.
	 * @param c
	 * @return
	 */
	public boolean getResult(int c)
	{
		if(c == 8 && hasPut(c)){
			drawBuffer();
			return true;
		}
		for(int i = 0; i < 8; ++i){
			if(check(c, i)){
				board[c][i] = true;
				getResult(++c);
			}
		}
		return false;
	}
	
	/**
	 * Judge whether column c has be put a queue
	 * @param c
	 * @return
	 */
	private boolean hasPut(int c)
	{
		for(int i = 0; i < 8; ++i){
			if(board[c][i]){
				return true;
			}
		}
		return false;
	}
	
	private void drawBuffer()
	{
		calculateSize();
		
		Graphics2D g2d = (Graphics2D) buffer.getGraphics();
		//clear
		g2d.clearRect(0, 0, gridWidth, gridHeight);
		
		g2d.setColor(Color.BLACK);
		for (int i = 0; i < 9; ++i) {
			g2d.drawLine(margin, margin + i * gridh
					, gridWidth + margin
					, margin + i * gridh);

			g2d.drawLine(margin + i * gridw, margin
					, margin + i * gridw
					, gridHeight + margin);
		}
		for(int i = 0; i < 8; ++i){
			for(int j = 0; j < 8; ++j){
				if(board[i][j]){
					drawChess(i, j, g2d);
				}
			}
		}
		repaint();
	}

	/**
	 * Draw a chess in (x,y)
	 * @param x
	 * @param y
	 * @param g2d
	 */
	private void drawChess(int x, int y, Graphics2D g2d)
	{
		int radius = gridh > gridw ? gridw : gridh;
		radius -= 10;
		int cx = x * gridw + ( gridw - radius ) / 2 + margin;
		int cy = y * gridh + ( gridh - radius ) / 2 + margin;
		Color oldc = g2d.getColor();
		g2d.setColor(color[x][y]);
		g2d.fillOval(cx, cy, radius, radius);
		g2d.setColor(oldc);
	}
	
	/**
	 * Calculate the value of some members
	 */
	private void calculateSize()
	{
		int w = this.getWidth();
		int h = this.getHeight();

		margin = 30;
		infoW = 50;

		gridWidth = (w - 2 * margin) & (~0x7);
		gridHeight = (h - 3 * margin - infoW) & (~0x7);
		gridw = gridWidth / 8;
		gridh = gridHeight / 8;// We need show some
					// info at the bottom
	}
	
	/**
	 * Check if the queue can be put in (x,y)
	 * @param x
	 * @param y
	 * @return true means can, or can not
	 */
	private boolean check(int x, int y)
	{
		for (int i = 0; i < 8; ++i) {
			//left and right
			if (i != x && board[i][y]) {
				vx = i;
				vy = y;
				return false;
			}
			// up and down
			if (i != y && board[x][i]) {
				vx = x;
				vy = i;
				return false;
			}
			//leftup
			if (i != 0 && x - i >= 0 && y - i >= 0
					&& board[x - i][y - i]) {
				vx = x - i;
				vy = y - i;
				return false;
			}
			//rightdown
			if (i != 0 && x + i < 8 && y + i < 8
					&& board[x + i][y + i]) {
				vx = x + i;
				vy = y + i;
				return false;
			}
			//leftdown
			if (i != 0 && x - i >= 0 && y + i < 8
					&& board[x - i][y + i]) {
				vx = x - i;
				vy = y + i;
				return false;
			}
			//rightup
			if (i != 0 && x + i < 8 && y - i >= 0
					&& board[x + i][y - i]) {
				vx = x + i;
				vy = y - i;
				return false;
			}
			
		}
		return true;
	}
	
	private int gridw, gridh;
	private int gridWidth, gridHeight;
	private int margin;
	private int infoW;
	private Image buffer = null;
	private Date startTime, endTime;
	
	private boolean[][] board = new boolean[8][8];
	private Color[][] color = new Color[8][8];
	private int cnt;
	private int vx, vy;
	private Timer timer = new Timer(true);
	
	private Board me = this;

	private class MyMouseListener extends MouseAdapter
	{
		public void mouseClicked(MouseEvent e)
		{
			if(e.getButton() != MouseEvent.BUTTON1){
				return;
			}
			int x = (e.getX() - margin) / gridw;
			int y = (e.getY() - margin) / gridh;
			
			board[x][y] = !board[x][y];
			
			if(!check(x, y)){
				color[vx][vy] = Color.RED;
				drawBuffer();
				JOptionPane.showMessageDialog(me
						, "Invalidate Position!"
						+ "(" + vx + "," + vy + ")"
						, "Wrong!!"
						, JOptionPane.WARNING_MESSAGE);
				color[vx][vy] = Color.BLACK;
				board[x][y] = !board[x][y];
			}
			drawBuffer();
		}

		public void mousePressed(MouseEvent e)
		{
		}

		public void mouseReleased(MouseEvent e)
		{
		}
	}
}
