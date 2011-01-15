package problem5;

import java.awt.BorderLayout;
import java.awt.Toolkit;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.text.ParseException;
import java.text.SimpleDateFormat;
import java.util.Date;

import javax.swing.JButton;
import javax.swing.JFrame;
import javax.swing.JLabel;
import javax.swing.JOptionPane;
import javax.swing.JPanel;
import javax.swing.JScrollPane;
import javax.swing.JTabbedPane;
import javax.swing.JTable;
import javax.swing.JTextField;
import javax.swing.ListSelectionModel;

public class MainFrame extends JFrame
{
	public MainFrame() {
		this.setTitle("Income and Payout");
		this.setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);

		this.setSize(Toolkit.getDefaultToolkit().getScreenSize());

		data = new IncomeAndPayoutInfo();
		
		tabpane = new JTabbedPane();
		this.getContentPane().add(tabpane, BorderLayout.CENTER);
		ptm = new PayoutTableModel(data);
		itm = new IncomeTableModel(data);
		pt = new JTable(ptm);
		it = new JTable(itm);
		pt.setSelectionMode(
				ListSelectionModel.SINGLE_INTERVAL_SELECTION);
		it.setSelectionMode(
				ListSelectionModel.SINGLE_INTERVAL_SELECTION);
		
		JScrollPane jsp1 = new JScrollPane(it);
		JScrollPane jsp2 = new JScrollPane(pt);
		jsp1.setHorizontalScrollBarPolicy(
				JScrollPane.HORIZONTAL_SCROLLBAR_AS_NEEDED);
		jsp1.setVerticalScrollBarPolicy(
				JScrollPane.VERTICAL_SCROLLBAR_ALWAYS);
		jsp2.setHorizontalScrollBarPolicy(
				JScrollPane.HORIZONTAL_SCROLLBAR_AS_NEEDED);
		jsp2.setVerticalScrollBarPolicy(
				JScrollPane.VERTICAL_SCROLLBAR_ALWAYS);
		
		tabpane.addTab("Income", jsp1);
		tabpane.addTab("Payout", jsp2);
		
		
		JPanel p = new JPanel();
		addPayoutBtn = new JButton("Add Payout");
		addPayoutBtn.addActionListener(new ActionListener()
		{

			@Override
			public void actionPerformed(ActionEvent e)
			{
				// TODO Auto-generated method stub
				tabpane.setSelectedIndex(1);
				MoneyItem i = new MoneyItem();
				ptm.addPayout(i);
			}
		});
		addIncomeBtn = new JButton("Add Income");
		addIncomeBtn.addActionListener(new ActionListener()
		{

			@Override
			public void actionPerformed(ActionEvent e)
			{
				// TODO Auto-generated method stub
				tabpane.setSelectedIndex(0);
				MoneyItem i = new MoneyItem();
				itm.addIncome(i);
			}
		});

		delBtn = new JButton("Delete");
		delBtn.addActionListener(new ActionListener()
		{
			
			@Override
			public void actionPerformed(ActionEvent e)
			{
				// TODO Auto-generated method stub
				int tab = tabpane.getSelectedIndex();
				if(tab == 0){
					itm.delIncome(it.getSelectedRows());
				}else if(tab == 1){
					ptm.delPayout(pt.getSelectedRows());
				}
			}
		});
		p.add(addIncomeBtn);
		p.add(addPayoutBtn);
		p.add(delBtn);
		staBtn = new JButton("Statistics");
		staBtn.addActionListener(new ActionListener()
		{
			
			@Override
			public void actionPerformed(ActionEvent e)
			{
				// TODO Auto-generated method stub
				Date from = null, to = null;
				try {
					from = sdf.parse(fromTime.getText());
					to = sdf.parse(toTime.getText());
				}
				catch (ParseException e1) {
					// TODO Auto-generated catch block
					JOptionPane.showMessageDialog(me
							, "Wrong data format!!"
							+ "yyyy/MM/dd"
							, "Error"
							, JOptionPane
								.ERROR_MESSAGE);
					return;
				}
				double income = 0.0, payout = 0.0;
				income = data.staticticsIncome(from, to);
				payout = data.staticticsPayout(from, to);
				JOptionPane.showMessageDialog(me,
						"Income : " + income + "\n" +
						"Payout : " + payout
						, "Statictiss"
						, JOptionPane.
							INFORMATION_MESSAGE);
			}
		});
		p.add(staBtn);
		p.add(new JLabel("From:"));
		fromTime = new JTextField(10);
		p.add(fromTime);
		p.add(new JLabel("To:"));
		toTime = new JTextField(10);
		p.add(toTime);
		this.getContentPane().add(p, BorderLayout.SOUTH);

	}

	private JTabbedPane tabpane;
	private JTable it, pt;
	private JButton staBtn;
	private JTextField fromTime, toTime;
	private JButton addPayoutBtn, addIncomeBtn;
	private JButton delBtn;
	private PayoutTableModel ptm;
	private IncomeTableModel itm;
	private IncomeAndPayoutInfo data;
	private static SimpleDateFormat sdf 
		= new SimpleDateFormat("yyyy/MM/dd");
	
	private MainFrame me = this;
}
