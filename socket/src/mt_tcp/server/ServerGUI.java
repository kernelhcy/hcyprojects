package mt_tcp.server;

import java.awt.BorderLayout;
import java.awt.FlowLayout;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import javax.swing.BorderFactory;
import javax.swing.JButton;
import javax.swing.JFrame;
import javax.swing.JLabel;
import javax.swing.JPanel;
import javax.swing.JScrollPane;
import javax.swing.JTextArea;

public class ServerGUI
{
	private ServerGUI self = this;
	private JFrame mainFrame = null;
	private JPanel mainPanel = null;
	private JPanel panel1 = null;
	private JPanel panel2 = null;
	private FlowLayout fl1 = null;
	private FlowLayout fl2 = null;
	private BorderLayout bl = null;
	private JButton startButton  = null;
	private JButton stopButton = null;
	private JButton exitButton = null;
	private JLabel ctlabel = null;
	private JLabel currThreadCnt = null;
	private MainAcceptThread server = null;
	private JTextArea info = null;
	private JScrollPane jsp = null;
	
	public ServerGUI()
	{
		System.out.println("create window");
		mainFrame = new JFrame();
		mainFrame.setTitle("Server");
		
		bl = new BorderLayout(2,1);
		mainPanel = new JPanel(bl);
		fl1 = new FlowLayout();
		fl1.setAlignment(FlowLayout.CENTER);
		fl2 = new FlowLayout();
		fl2.setAlignment(FlowLayout.CENTER);
		panel1 = new JPanel(fl1);
		panel2 = new JPanel(fl2);
		
		mainPanel.add(panel1,BorderLayout.NORTH);
		mainPanel.add(panel2,BorderLayout.CENTER);
		
		info = new JTextArea("");
		info.setLineWrap(true);
		info.setRows(10);
		info.setEditable(false);
		jsp = new JScrollPane(info);
		jsp.setHorizontalScrollBarPolicy(JScrollPane.HORIZONTAL_SCROLLBAR_NEVER);
		jsp.setVerticalScrollBarPolicy(JScrollPane.VERTICAL_SCROLLBAR_AS_NEEDED);
		jsp.setBorder(BorderFactory.createTitledBorder("Info:"));
		mainPanel.add(jsp, BorderLayout.SOUTH);
		
		mainFrame.getContentPane().add(mainPanel);
		
		ctlabel = new JLabel("Current Threads Count:");
		panel1.add(ctlabel);
		
		currThreadCnt = new JLabel("0");
		panel1.add(currThreadCnt);
		
		/*
		 * 设置线程，实时更新显示的线程数量。
		 */
		Thread updateThreadCnt = new Thread(new Runnable(){

            @Override
			public void run()
			{
				while (true)
				{
					//currThreadCnt.setText(String.valueOf(TransferThreadServer.getCnt()));
					try
					{
						Thread.sleep(5);
					}
					catch (InterruptedException e)
					{
						// TODO Auto-generated catch block
						e.printStackTrace();
					}
				}
			}
			
		});
		//设置为守护线程
		updateThreadCnt.setDaemon(true);
		updateThreadCnt.start();
		
		startButton = new JButton("Start");
		startButton.addActionListener(new ActionListener(){

			public void actionPerformed(ActionEvent e)
			{
				
//				System.out.println("start...");
				startButton.setEnabled(false);
				stopButton.setEnabled(true);
				
				server = new MainAcceptThread(1234);
				server.setServerGui(self);
				Thread listenThread = new Thread(new Runnable(){

					public void run()
					{
						server.acceptConnection();
					}
					
				});
				listenThread.start();
				
			}
			
		});
		
		stopButton = new JButton("Stop");
		stopButton.setEnabled(false);
		stopButton.addActionListener(new ActionListener(){

			
			public void actionPerformed(ActionEvent e)
			{
//				System.out.println("stop...");
				server.stop();
				startButton.setEnabled(true);
				stopButton.setEnabled(false);
			}
			
		});
		
		exitButton = new JButton("Exit");
		exitButton.addActionListener(new ActionListener(){

			
			public void actionPerformed(ActionEvent e)
			{
//				System.out.println("exit...");
				if (server != null)
				{
					server.stop();
				}
				mainFrame.dispose();
				
			}
			
		});
		
		
		
		panel2.add(startButton);
		panel2.add(stopButton);
		panel2.add(exitButton);
		
		panel1.setBorder(BorderFactory.createEmptyBorder());
		
		mainFrame.setDefaultCloseOperation(JFrame.DO_NOTHING_ON_CLOSE);
		
		mainFrame.setSize(500, 300);
		mainFrame.setVisible(true);
	}
	
	public void writeInfo(String information)
	{
		this.info.append(information);
	}
	
	
	public static void main(String[] args)
	{
		new ServerGUI();
	}
}
