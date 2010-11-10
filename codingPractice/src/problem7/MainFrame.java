package problem7;

import java.awt.BorderLayout;
import java.awt.Cursor;
import java.awt.event.AdjustmentEvent;
import java.awt.event.AdjustmentListener;
import java.awt.geom.Line2D;

import javax.swing.JButton;
import javax.swing.JFrame;
import javax.swing.JScrollBar;
import javax.swing.JScrollPane;
import javax.swing.JToolBar;

public class MainFrame extends JFrame
{
	public MainFrame()
	{
		setTitle("画图Demo");
		setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);
		toolbar = createToolbar();
		getContentPane().add(toolbar, BorderLayout.NORTH);
		
		setSize(700, 500);
		setVisible(true);
		
		dp = new DrawPanel();
		JScrollPane jsp = new JScrollPane(dp);
		jsp.setVerticalScrollBarPolicy(
				JScrollPane.VERTICAL_SCROLLBAR_ALWAYS);
		jsp.setHorizontalScrollBarPolicy(
				JScrollPane.HORIZONTAL_SCROLLBAR_ALWAYS);
				
		getContentPane().add(jsp, BorderLayout.CENTER);
		dp.repaint();
		
	}
	
	private JToolBar createToolbar()
	{
		toolbar = new JToolBar();
		toolbar.setCursor(new Cursor(Cursor.HAND_CURSOR));
		toolbar.setFloatable(false); //不浮动
		lineBtn = new JButton("Create Line");
		
		rectBtn = new JButton("Create Rect");
		undoBtn = new JButton("Undo");
		redoBtn = new JButton("Redo");
		toolbar.add(lineBtn);
		toolbar.add(undoBtn);
		toolbar.add(redoBtn);
		return toolbar;
	}
	
	private JToolBar toolbar;
	private JButton lineBtn, rectBtn, undoBtn, redoBtn;
	private DrawPanel dp;
}
