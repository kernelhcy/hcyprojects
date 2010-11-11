package problem7;

import java.awt.BorderLayout;
import java.awt.Color;
import java.awt.Cursor;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.io.File;

import javax.swing.ButtonGroup;
import javax.swing.JButton;
import javax.swing.JCheckBox;
import javax.swing.JColorChooser;
import javax.swing.JFileChooser;
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
		
		elliBtn = new JRadioButton("Elli");
		elliBtn.addActionListener(ac);
		bg.add(elliBtn);
		
		polyBtn = new JRadioButton("Poly");
		polyBtn.addActionListener(ac);
		bg.add(polyBtn);
		
		undoBtn = new JButton("Undo");
		undoBtn.addActionListener(ac);
		redoBtn = new JButton("Redo");
		redoBtn.addActionListener(ac);
		saveBtn = new JButton("Save");
		saveBtn.addActionListener(ac);
		openBtn = new JButton("Open");
		openBtn.addActionListener(ac);
		
		fillcb = new JCheckBox("Fill");
		fillcb.addActionListener(ac);
		fillcb.setSelected(false);
		
		toolbar.add(nothingBtn);
		toolbar.add(lineBtn);
		toolbar.add(rectBtn);
		toolbar.add(elliBtn);
		toolbar.add(polyBtn);
		
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
		toolbar.add(openBtn);
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
			}else if(src == elliBtn){
				dp.setShapeListener(ShapeType.ELLI
						, fillcb.isSelected()
						, colorBtn.getColor());
			}else if(src == polyBtn){
				dp.setShapeListener(ShapeType.POLYGON
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
			}else if(src == saveBtn){
				JFileChooser fc = new JFileChooser(".");
				int re = fc.showSaveDialog(mf);
				if(re == JFileChooser.APPROVE_OPTION){
					File f = fc.getSelectedFile();
					try {
						dp.writeToFile(f);
					}
					catch (Exception e1) {
						// TODO Auto-generated catch block
						e1.printStackTrace();
					}
				}
			}else if(src == openBtn){
				JFileChooser fc = new JFileChooser(".");
				int re = fc.showOpenDialog(mf);
				if(re == JFileChooser.APPROVE_OPTION){
					File f = fc.getSelectedFile();
					try {
						dp.readFromFile(f);
					}
					catch (Exception e1) {
						// TODO Auto-generated catch block
						e1.printStackTrace();
					}
				}
			}
			
		}
		
		//redo button action
		private void redo()
		{
			MyShape tmp = dp.lastShape();
			dp.addShape(preshape);
			preshape = tmp;
			dp.repaint();
		}
		
		//undo button action
		private void undo()
		{
			preshape = dp.deleteShape(dp.lastShape());
			dp.repaint();
			System.out.println("Undo" + preshape);
		}
	}
	
	private JToolBar toolbar;
	private ActionListener ac;
	private ButtonGroup bg;
	private JRadioButton lineBtn, rectBtn, elliBtn, polyBtn, nothingBtn;
	private JButton undoBtn, redoBtn, clearBtn, saveBtn, openBtn;
	private ColorButton colorBtn;
	private DrawPanel dp;
	private JCheckBox fillcb;
	private MainFrame mf = this;
	
	private MyShape preshape = null;
}
