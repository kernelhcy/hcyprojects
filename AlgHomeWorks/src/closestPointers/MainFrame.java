package closestPointers;

import java.awt.BorderLayout;
import java.awt.Color;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.util.ArrayList;
import java.util.Iterator;

import javax.swing.BorderFactory;
import javax.swing.ButtonGroup;
import javax.swing.JButton;
import javax.swing.JCheckBox;
import javax.swing.JComboBox;
import javax.swing.JFrame;
import javax.swing.JPanel;
import javax.swing.JRadioButton;

public class MainFrame extends JFrame
{
	public MainFrame()
	{
		this.setDefaultCloseOperation(EXIT_ON_CLOSE);
		this.setTitle("Get The Closest Pointers");
		
		panel = new PaintPanel();
		layout = new BorderLayout();
		
		runBtn = new JButton("Get Them!");
		runBtn.addActionListener(new ActionListener()
		{
			
			@Override
			public void actionPerformed(ActionEvent e) {
				// TODO Auto-generated method stub
				panel.setLocked(true);
				runBtn.setEnabled(false);
				runBtn.setText("Getting them...");
				ArrayList<PaintPoint> pps = panel.getPoints();
				if(pps == null || pps.size() == 0){
					panel.setLocked(false);
					runBtn.setEnabled(true);
					runBtn.setText("Get Them!");
					return;
				}
				DPoint[] dps = new DPoint[pps.size()];
				for(int i = 0; i < dps.length; ++i){
					dps[i] = new DPoint();
					dps[i].x = pps.get(i).x;
					dps[i].y = pps.get(i).y;
				}
				
				ClosestPointersGenerator cpg = 
					new ClosestPointersGenerator(dps);
				cpg.run(method);
				CPResult re = cpg.getResult();
				
				System.out.println("Result: " + re.p1 + re.p2);
				panel.showResult((int)re.p1.x, (int)re.p1.y,
						(int)re.p2.x, (int)re.p2.y);
				runBtn.setEnabled(true);
				panel.setLocked(false);
				runBtn.setText("Get Them!");
			}
		});
		
		clearBtn = new JButton("Clear");
		clearBtn.addActionListener(new ActionListener()
		{
			
			@Override
			public void actionPerformed(ActionEvent e) {
				// TODO Auto-generated method stub
				panel.clear();
				runBtn.setEnabled(true);
				runBtn.setText("Get Them!");
			}
		});
		
		
		resetBtn = new JButton("Reset");
		resetBtn.addActionListener(new ActionListener()
		{
			
			@Override
			public void actionPerformed(ActionEvent e) {
				// TODO Auto-generated method stub
				PaintPoint p;
				Iterator<PaintPoint> it 
				= panel.getPoints().iterator();
				while(it.hasNext()){
					p = it.next();
					p.color = Color.BLUE;
					p.radius = 7;
				}
				panel.repaint();
				runBtn.setEnabled(true);
				runBtn.setText("Get Them!");
			}
		});
		
		btnPanel = new JPanel();
		//btnPanel.setBorder(BorderFactory.createLineBorder(Color.BLUE));
		RadioButtonListener rbListener = new RadioButtonListener();
		
		JRadioButton dcRBtn = new JRadioButton("Divide & Conquer");
		dcRBtn.setActionCommand(DCMETHOD);
		dcRBtn.addActionListener(rbListener);
		dcRBtn.setSelected(true);
		this.method = ClosestPointersGenerator.DC;
		
		JRadioButton nRBtn = new JRadioButton("Normal");
		nRBtn.setActionCommand(NORMETHOD);
		nRBtn.addActionListener(rbListener);
		ButtonGroup bg = new ButtonGroup();
		bg.add(nRBtn);
		bg.add(dcRBtn);
		
		showCoordinateCb = new JCheckBox("Show CoordinateCb");
		showCoordinateCb.addActionListener(new ActionListener()
		{
			
			@Override
			public void actionPerformed(ActionEvent e) {
				// TODO Auto-generated method stub
				panel.setShowCoordinate(
				((JCheckBox)e.getSource()).isSelected());
			}
		});
		showCoordinateCb.setSelected(panel.isShowCoordinate());
		
		btnPanel.add(nRBtn);
		btnPanel.add(dcRBtn);
		btnPanel.add(runBtn);
		btnPanel.add(clearBtn);
		btnPanel.add(resetBtn);
		btnPanel.add(showCoordinateCb);
		
		this.getContentPane().setLayout(layout);
		this.getContentPane().add(panel, BorderLayout.CENTER);
		this.getContentPane().add(btnPanel, BorderLayout.SOUTH);

		this.setSize(1000, 700);
		this.setVisible(true);
	}
	
	/**
	 * raido button 的监听器
	 * @author hcy
	 *
	 */
	private class RadioButtonListener implements ActionListener
	{

		@Override
		public void actionPerformed(ActionEvent e) {
			// TODO Auto-generated method stub
			String cmd = e.getActionCommand();
			if (cmd.equals(DCMETHOD)){
				method = ClosestPointersGenerator.DC;
			}
			else if(cmd.equals(NORMETHOD)){
				method = ClosestPointersGenerator.NORMAL;
			}
		}
		
	}
	
	private PaintPanel panel;
	private BorderLayout layout;
	private JButton runBtn, clearBtn, resetBtn;
	private JPanel btnPanel;
	private int method;
	private final String DCMETHOD = "DC";
	private final String NORMETHOD = "NOR";
	private JCheckBox showCoordinateCb;
}
