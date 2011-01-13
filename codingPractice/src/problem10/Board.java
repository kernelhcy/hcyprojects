package problem10;

import java.awt.Color;
import java.awt.Graphics;
import java.awt.Point;
import java.awt.event.MouseEvent;
import java.awt.event.MouseListener;

import javax.swing.JOptionPane;
import javax.swing.JPanel;

public class Board extends JPanel implements MouseListener
{
	public Board(InfoPanel ip) {
		this.ip = ip;
		start = new Point(10, 10);
		end = new Point(410, 410);

		chess = new boolean[8][8];
		colors = new Color[8][8];
		nextP = new boolean[8][8];
		blackPlayer = true;

		this.addMouseListener(this);
	}

	public void start()
	{
		chess = new boolean[8][8];
		nextP = new boolean[8][8];
		colors = new Color[8][8];
		blackPlayer = true;

		// 初始的四个棋子
		chess[3][3] = chess[4][4] = true;
		colors[3][3] = colors[4][4] = Color.white;
		chess[3][4] = chess[4][3] = true;
		colors[3][4] = colors[4][3] = Color.BLACK;

		blackPlayer = true;
		checkHasPlace();

		repaint();
	}

	public void restart()
	{
		start();
	}

	@Override
	public void paintComponent(Graphics g)
	{
		// 背景
		Color oldc = g.getColor();
		g.setColor(Color.LIGHT_GRAY);
		g.fillRect(0, 0, this.getWidth(), this.getHeight());
		g.setColor(oldc);

		paintGrid(g);
		paintChesses(g);
		paintHint(g);
	}

	/**
	 * 检查当前玩家可以放子的地方。
	 */
	private boolean checkHasPlace()
	{
		Color cc;
		if (blackPlayer) {
			cc = Color.BLACK;
		}
		else {
			cc = Color.WHITE;
		}
		for (int i = 0; i < 8; ++i) {
			for (int j = 0; j < 8; ++j) {
				nextP[i][j] = false;
			}
		}
		for (int i = 0; i < 8; ++i) {
			for (int j = 0; j < 8; ++j) {
				if (chess[i][j] && colors[i][j] == cc) {
					checkOneChess(i, j);
				}
			}
		}

		for (int i = 0; i < 8; ++i) {
			for (int j = 0; j < 8; ++j) {
				if (nextP[i][j]) {
					return true;
				}
			}
		}

		return false;
	}

	/**
	 * 检查基于x，y处的棋子有几处可以放子
	 * 
	 * @param x
	 * @param y
	 * @return
	 */
	private int checkOneChess(int x, int y)
	{
		int cnt = 0;
		int tx, ty;
		boolean hasNext = false;

		// 下面
		tx = x;
		ty = y + 1;
		while (ty < 8 && chess[tx][ty]
				&& colors[tx][ty] != colors[x][y]) {
			++ty;
			hasNext = true;
		}
		if (hasNext && ty != 8 && !chess[tx][ty]) {
			++cnt;
			nextP[tx][ty] = true;
		}

		// 上面
		hasNext = false;
		tx = x;
		ty = y - 1;
		while (ty >= 0 && chess[tx][ty]
				&& colors[tx][ty] != colors[x][y]) {
			--ty;
			hasNext = true;
		}
		if (hasNext && ty != -1 && !chess[tx][ty]) {
			++cnt;
			nextP[tx][ty] = true;
		}

		// 左面
		hasNext = false;
		tx = x - 1;
		ty = y;
		while (tx >= 0 && chess[tx][ty]
				&& colors[tx][ty] != colors[x][y]) {
			--tx;
			hasNext = true;
		}
		if (hasNext && tx != -1 && !chess[tx][ty]) {
			++cnt;
			nextP[tx][ty] = true;
		}

		// 右面
		hasNext = false;
		tx = x + 1;
		ty = y;
		while (tx < 8 && chess[tx][ty]
				&& colors[tx][ty] != colors[x][y]) {
			++tx;
			hasNext = true;
		}
		if (hasNext && tx != 8 && !chess[tx][ty]) {
			++cnt;
			nextP[tx][ty] = true;
		}

		// 左上面
		hasNext = false;
		tx = x - 1;
		ty = y - 1;
		while (ty >= 0 && tx >= 0 && chess[tx][ty]
				&& colors[tx][ty] != colors[x][y]) {
			--tx;
			--ty;
			hasNext = true;
		}
		if (hasNext && ty != -1 && tx != -1 && !chess[tx][ty]) {
			++cnt;
			nextP[tx][ty] = true;
		}

		// 右下面
		hasNext = false;
		tx = x + 1;
		ty = y + 1;
		while (ty < 8 && tx < 8 && chess[tx][ty]
				&& colors[tx][ty] != colors[x][y]) {
			++tx;
			++ty;
			hasNext = true;
		}
		if (hasNext && ty != 8 && tx != 8 && !chess[tx][ty]) {
			++cnt;
			nextP[tx][ty] = true;
		}

		// 右上面
		hasNext = false;
		tx = x + 1;
		ty = y - 1;
		while (ty >= 0 && tx < 8 && chess[tx][ty]
				&& colors[tx][ty] != colors[x][y]) {
			++tx;
			--ty;
			hasNext = true;
		}
		if (hasNext && ty != -1 && tx != 8 && !chess[tx][ty]) {
			++cnt;
			nextP[tx][ty] = true;
		}

		// 左下面
		hasNext = false;
		tx = x - 1;
		ty = y + 1;
		while (ty < 8 && tx >= 0 && chess[tx][ty]
				&& colors[tx][ty] != colors[x][y]) {
			--tx;
			++ty;
			hasNext = true;
		}
		if (hasNext && ty != 8 && tx != -1 && !chess[tx][ty]) {
			++cnt;
			nextP[tx][ty] = true;
		}

		return cnt;
	}

	/**
	 * 检查游戏是否结束
	 * 
	 * @return
	 */
	private boolean checkOver()
	{
		wn = bn = 0;
		for (int i = 0; i < 8; ++i) {
			for (int j = 0; j < 8; ++j) {
				if (chess[i][j]) {
					if (colors[i][j] == Color.BLACK) {
						++bn;
					}
					else if (colors[i][j] == Color.WHITE) {
						++wn;
					}
					if (bn + wn == 64) {
						return true;
					}
				}
			}
		}
		if(wn == 0 || bn == 0){
			return true;
		}
		return false;
	}

	/**
	 * 画棋盘
	 * 
	 * @param g
	 */
	private void paintGrid(Graphics g)
	{
		int w, h;
		w = (end.x - start.x) / 8;
		h = (end.y - start.y) / 8;
		gridW = w;
		gridH = h;
		int tmp = start.x;

		// 画棋盘
		for (int i = 0; i < 9; ++i) {
			g.drawLine(tmp, start.y, tmp, end.y);
			tmp += w;
		}
		tmp = start.y;
		for (int i = 0; i < 9; ++i) {
			g.drawLine(start.x, tmp, end.x, tmp);
			tmp += h;
		}
	}

	/**
	 * 画所有的棋子
	 */
	private void paintChesses(Graphics g)
	{
		Color oldc;
		for (int i = 0; i < 8; ++i) {
			for (int j = 0; j < 8; ++j) {
				if (chess[i][j]) {
					oldc = g.getColor();
					g.setColor(colors[i][j]);
					paintChess(g, i, j);
					g.setColor(oldc);
				}
			}
		}
	}

	/**
	 * 画一个棋子
	 * 
	 * @param g
	 * @param x
	 * @param y
	 */
	private void paintChess(Graphics g, int i, int j)
	{
		int x, y, r;
		x = gridW * i + 13;
		y = gridH * j + 13;
		r = gridW - 6;
		g.fillOval(x, y, r, r);
	}

	/**
	 * 显示提示
	 * 
	 * @param g
	 */
	private void paintHint(Graphics g)
	{
		Color oc = g.getColor();
		g.setColor(Color.RED);
		for (int i = 0; i < 8; ++i) {
			for (int j = 0; j < 8; ++j) {
				if (nextP[i][j]) {
					int x, y, r;
					x = gridW * i + 30;
					y = gridH * j + 30;
					r = gridW - 40;
					g.fillOval(x, y, r, r);
				}
			}
		}
		g.setColor(oc);
	}

	/**
	 * 吃掉对方的棋子
	 * 
	 * @param i
	 * @param j
	 */
	private void eatChesses(int x, int y)
	{
		int cnt = 0;
		int tx, ty;
		int ttx, tty;
		Color cc;
		boolean hasNext = false;
		//玩家已经换了
		if (!blackPlayer) {
			cc = Color.BLACK;
		}
		else {
			cc = Color.WHITE;
		}

		// 下面
		tx = x;
		ty = y + 1;
		while (ty < 8 && chess[tx][ty]
				&& colors[tx][ty] != colors[x][y]) {
			++ty;
			hasNext = true;
		}
		if ( ty != 8 && hasNext && chess[tx][ty]
				&& colors[tx][ty] == colors[x][y]) {
			ttx = x;
			tty = y + 1;
			while (tty < ty) {
				colors[ttx][tty] = cc;
				++tty;
			}
		}

		// 上面
		hasNext = false;
		tx = x;
		ty = y - 1;
		while (ty >= 0 && chess[tx][ty]
				&& colors[tx][ty] != colors[x][y]) {
			--ty;
			hasNext = chess[tx][ty];
		}
		if (ty != -1 && hasNext && chess[tx][ty]
				&& colors[tx][ty] == colors[x][y]) {
			ttx = x;
			tty = y - 1;
			while (tty > ty) {
				colors[ttx][tty] = cc;
				--tty;
			}
		}

		// 左面
		hasNext = false;
		tx = x - 1;
		ty = y;
		while (tx >= 0 && chess[tx][ty]
				&& colors[tx][ty] != colors[x][y]) {
			--tx;
			hasNext = true;
		}
		if (tx != -1 && hasNext && chess[tx][ty]
				&& colors[tx][ty] == colors[x][y]) {
			ttx = x - 1;
			tty = y;
			while (ttx > tx) {
				colors[ttx][tty] = cc;
				--ttx;
			}
		}

		// 右面
		hasNext = false;
		tx = x + 1;
		ty = y;
		while (tx < 8 && chess[tx][ty]
				&& colors[tx][ty] != colors[x][y]) {
			++tx;
			hasNext = true;
		}
		if (tx != 8 && hasNext && chess[tx][ty]
				&& colors[tx][ty] == colors[x][y]) {
			ttx = x + 1;
			tty = y;
			while (ttx < tx) {
				colors[ttx][tty] = cc;
				++ttx;
			}
		}

		// 左上面
		hasNext = false;
		tx = x - 1;
		ty = y - 1;
		while (ty >= 0 && tx >= 0 && chess[tx][ty]
				&& colors[tx][ty] != colors[x][y]) {
			--tx;
			--ty;
			hasNext = true;
		}
		if (ty != -1 && tx != -1 && hasNext && chess[tx][ty]
				&& colors[tx][ty] == colors[x][y]) {
			ttx = x - 1;
			tty = y - 1;
			while (ttx > tx && tty > ty) {
				colors[ttx][tty] = cc;
				--ttx;
				--tty;
			}
		}

		// 右下面
		hasNext = false;
		tx = x + 1;
		ty = y + 1;
		while (ty < 8 && tx < 8 && chess[tx][ty]
				&& colors[tx][ty] != colors[x][y]) {
			++tx;
			++ty;
			hasNext = true;
		}
		if (ty != 8 && tx != 8 && hasNext && chess[tx][ty]
				&& colors[tx][ty] == colors[x][y]) {
			ttx = x + 1;
			tty = y + 1;
			while (ttx < tx && tty < ty) {
				colors[ttx][tty] = cc;
				++ttx;
				++tty;
			}
		}

		// 右上面
		hasNext = false;
		tx = x + 1;
		ty = y - 1;
		while (ty >= 0 && tx < 8 && chess[tx][ty]
				&& colors[tx][ty] != colors[x][y]) {
			++tx;
			--ty;
			hasNext = true;
		}
		if ( ty != -1 && tx != 8 && hasNext && chess[tx][ty] 
				&& colors[tx][ty] == colors[x][y]) {
			ttx = x + 1;
			tty = y - 1;
			while (ttx < tx && tty > ty) {
				colors[ttx][tty] = cc;
				++ttx;
				--tty;
			}
		}

		// 左下面
		hasNext = false;
		tx = x - 1;
		ty = y + 1;
		while (ty < 8 && tx >= 0 && chess[tx][ty]
				&& colors[tx][ty] != colors[x][y]) {
			--tx;
			++ty;
			hasNext = true;
		}
		if (ty != 8 && tx != -1 && hasNext && chess[tx][ty]
				&& colors[tx][ty] == colors[x][y]) {
			ttx = x - 1;
			tty = y + 1;
			while (ttx > tx && tty < ty) {
				colors[ttx][tty] = cc;
				--ttx;
				++tty;
			}
		}

	}

	@Override
	public void mouseClicked(MouseEvent e)
	{
		// TODO Auto-generated method stub
		Point pos = e.getPoint();
		pos.x -= start.x;
		;
		pos.y -= start.y;

		int x, y;
		x = pos.x / gridW;
		y = pos.y / gridH;

		// 不能放这
		if (!nextP[x][y]) {
			return;
		}
		
		if (!chess[x][y]) {
			if (blackPlayer) {
				colors[x][y] = Color.BLACK;
				blackPlayer = false;
			}
			else {
				colors[x][y] = Color.WHITE;
				blackPlayer = true;
			}
		}
		eatChesses(x, y);
		chess[x][y] = true;
		
		//下面的玩家无路可走。继续走。
		if(!checkHasPlace()){
			if(blackPlayer){
				blackPlayer = false;
			}else{
				blackPlayer = true;
			}
		}
		System.out.printf("Select (%d, %d)\n", x, y);
		
		ip.whiteNum.setText("" + wn);
		ip.blackNum.setText("" + bn);
		if(blackPlayer){
			ip.player.setText("Black");
		}else{
			ip.player.setText("White");
		}
		repaint();
		if(checkOver()){
			String msg = "";
			msg += "White:" + wn + "\n";
			msg += "Black:" + bn + "\n";
			msg += (wn > bn ? "White" : "Black") + " win!";
			JOptionPane.showMessageDialog(this, msg);
		}
	}

	private Point start, end; 	// 棋盘的左上角和右下角的位置。
	private int gridW, gridH; 	// 棋盘格子的长宽
	private boolean[][] chess; 	// 表示棋盘上棋子位置
	private boolean[][] nextP; 	// 当前玩家可放子的地方
	private Color[][] colors; 	// 对应棋子的颜色
	private InfoPanel ip;
	private int wn, bn;		// 各方的棋子数。

	private boolean blackPlayer; 	// 表示当前的玩家是黑还是白

	@Override
	public void mousePressed(MouseEvent e)
	{
		// TODO Auto-generated method stub

	}

	@Override
	public void mouseReleased(MouseEvent e)
	{
		// TODO Auto-generated method stub

	}

	@Override
	public void mouseEntered(MouseEvent e)
	{
		// TODO Auto-generated method stub

	}

	@Override
	public void mouseExited(MouseEvent e)
	{
		// TODO Auto-generated method stub

	}

}
