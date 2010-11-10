package problem7;

import java.awt.BorderLayout;
import java.awt.Color;
import java.awt.Cursor;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;

import javax.swing.ButtonGroup;
import javax.swing.JButton;
import javax.swing.JCheckBox;
import javax.swing.JColorChooser;
import javax.swing.JFrame;
import javax.swing.JLabel;
import javax.swing.JOptionPane;
import javax.swing.JRadioButton;
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
		//toolbar.setFloatable(false); //不浮动
		
		bg = new ButtonGroup();
		ac = new BtnListener();
		
		nothingBtn = new JRadioButton("Nothing");
		nothingBtn.addActionListener(ac);
		bg.add(nothingBtn);
		nothingBtn.setSelected(true);
		
		lineBtn = new JRadioButton("Line");
		lineBtn.addActionListener(ac);
		bg.add(lineBtn);
		
		rectBtn = new JRadioButton("Rect");
		rectBtn.addActionListener(ac);
		bg.add(rectBtn);
		
		undoBtn = new JButton("Undo");
		redoBtn = new JButton("Redo");
		saveBtn = new JButton("Save");
		
		fillcb = new JCheckBox("Fill");
		fillcb.addActionListener(ac);
		fillcb.setSelected(false);
		
		toolbar.add(nothingBtn);
		toolbar.add(lineBtn);
		toolbar.add(rectBtn);
		
		toolbar.addSeparator();
		toolbar.add(undoBtn);
		toolbar.add(redoBtn);
		
		toolbar.addSeparator();
		toolbar.add(fillcb);
		colorBtn = new ColorButton();
		colorBtn.addActionListener(new ActionListener()
		{
			
			@Override
			public void actionPerformed(ActionEvent e)
			{
				// TODO Auto-generated method stub
				Color c = JColorChooser.showDialog(mf
					, "Color", colorBtn.getColor());
				colorBtn.setColor(c);
				dp.setColor(c);
			}
		});
		toolbar.add(new JLabel(" Color:"));
		toolbar.add(colorBtn);
		
		toolbar.addSeparator();
		clearBtn = new JButton("Clear");
		clearBtn.addActionListener(new ActionListener()
		{
			
			@Override
			public void actionPerformed(ActionEvent e)
			{
				// TODO Auto-generated method stub
				int re = JOptionPane.showConfirmDialog(mf
						, "Clear all??");
				if(re == 0){
					dp.clear();
				}
			}
		});
		toolbar.add(clearBtn);
		toolbar.add(saveBtn);
		return toolbar;
	}
	
	
	private class BtnListener implements ActionListener
	{

		@Override
		public void actionPerformed(ActionEvent e)
		{
			// TODO Auto-generated method stub
			Object src = e.getSource();
			
			//通过事件源的地址确定事件源。
			if(src == lineBtn){
				dp.setShapeListener(ShapeType.LINE
						, false
						, colorBtn.getColor());
			}else if(src == rectBtn){
				dp.setShapeListener(ShapeType.RECT
						, fillcb.isSelected()
						, colorBtn.getColor());
			}else if(src == nothingBtn){
				dp.setShapeListener(ShapeType.NONE
						, fillcb.isSelected()
						, colorBtn.getColor());
			}else if(src == redoBtn){
				redo();
			}else if(src == undoBtn){
				undo();
			}else if(src == fillcb){
				dp.setFill(fillcb.isSelected());
			}
			
		}
		
		//redo button action
		private void redo()
		{
			
		}
		
		//undo button action
		private void undo()
		{
			
		}
	}
	
	private JToolBar toolbar;
	private ActionListener ac;
	private ButtonGroup bg;
	private JRadioButton lineBtn, rectBtn, nothingBtn;
	private JButton undoBtn, redoBtn, clearBtn, saveBtn;
	private ColorButton colorBtn;
	private DrawPanel dp;
	private JCheckBox fillcb;
	private MainFrame mf = this;
}
