package mt_tcp.server;

import java.awt.BorderLayout;
import java.awt.FlowLayout;
import java.awt.Font;
import java.awt.HeadlessException;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import javax.swing.BorderFactory;
import javax.swing.JButton;
import javax.swing.JFrame;
import javax.swing.JLabel;
import javax.swing.JOptionPane;
import javax.swing.JPanel;
import javax.swing.JScrollPane;
import javax.swing.JTextArea;
import javax.swing.JTextField;

public class ServerGUI
{

    public ServerGUI()
    {

        System.out.println("create window");
        mainFrame = new JFrame();
        mainFrame.setTitle("Server");

        bl = new BorderLayout(2, 1);
        mainPanel = new JPanel(bl);
        fl1 = new FlowLayout();
        fl1.setAlignment(FlowLayout.CENTER);
        fl1.setHgap(10);
        fl2 = new FlowLayout();
        fl2.setAlignment(FlowLayout.CENTER);
        fl2.setHgap(20);
        panel1 = new JPanel(fl1);
        panel2 = new JPanel(fl2);

        mainPanel.add(panel1, BorderLayout.NORTH);
        mainPanel.add(panel2, BorderLayout.CENTER);
        createInfoTextAera();

        mainFrame.getContentPane().add(mainPanel);

        label1 = new JLabel("Listening Port:");
        label1.setFont(new Font("Courier New", Font.PLAIN, 16));//"Courier New"
        panel1.add(label1);

        listenPort = new JTextField("1234");
        listenPort.setFont(new Font("Courier New", Font.PLAIN, 16));
        listenPort.setToolTipText("MUST BE greater than 1000 and less than 65535!");
        listenPort.setColumns(6);
        panel1.add(listenPort);

        JButton setDefaultPort = new JButton("Default");
        setDefaultPort.setFont(new Font("Courier New", Font.PLAIN, 12));
        setDefaultPort.addActionListener(new ActionListener() {

            @Override
            public void actionPerformed(ActionEvent e)
            {
                listenPort.setText("1234");
            }
        });
        panel1.add(setDefaultPort);

        startButton = new JButton("Start");
        startButton.setFont(new Font("Courier New", Font.PLAIN, 14));
        startButton.setToolTipText("Start the server.");
        startButton.addActionListener(new ActionListener()
        {

            public void actionPerformed(ActionEvent e)
            {

//				System.out.println("start...");
                
                int port = -1;
                try
                {
                    port = Integer.parseInt(listenPort.getText());
                }
                catch(NumberFormatException ee)
                {
                    showPortWarning("Bad port number!!\n"+ee.getMessage());
                    writeInfo("************************************************************\n");
                    writeInfo("Bad port number!!   "+ee.getMessage());
                    writeInfo("\n************************************************************\n");
                    return;
                }

                if (port < 1000 || port > 65535)
                {
                    showPortWarning("Bad port number!!\nPort number MUST BE greater than 1000 and less than 65535!");
                    writeInfo("************************************************************\n");
                    writeInfo("Bad port number!!\nPort number MUST BE greater than 1000 and less than 65535!");
                    writeInfo("\n************************************************************\n");
                    return;
                }

                writeInfo("Listenning Port Number：");
                writeInfo(listenPort.getText());
                writeInfo("\n");
                
                server = new MainAcceptThread(port);
                server.setServerGui(self);
                Thread listenThread = new Thread(new Runnable()
                {

                    public void run()
                    {
                        server.acceptConnection();
                    }
                });
                listenThread.start();
                startButton.setEnabled(false);
                stopButton.setEnabled(true);

            }

            /**
                *   提示端口错误！ 
                */
            private void showPortWarning(String msg) throws HeadlessException
            {
                System.out.println(msg);
                JOptionPane.showMessageDialog(mainFrame, msg, "Warning",
                    JOptionPane.WARNING_MESSAGE, null);
            }
        });

        stopButton = new JButton("Stop");
        stopButton.setToolTipText("Stop the server.");
        stopButton.setFont(new Font("Courier New", Font.PLAIN, 14));
        stopButton.setEnabled(false);
        stopButton.addActionListener(new ActionListener()
        {

            public void actionPerformed(ActionEvent e)
            {
//				System.out.println("stop...");
                server.stop();
                startButton.setEnabled(true);
                stopButton.setEnabled(false);
            }
        });

        exitButton = new JButton("Exit");
        exitButton.setFont(new Font("Courier New", Font.PLAIN, 14));
        exitButton.setToolTipText("Stop the server and exit.");
        exitButton.addActionListener(new ActionListener()
        {

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

        mainFrame.setSize(windowWidth, windowHeight);
        mainFrame.setVisible(true);
    }

    /**
     * 向消息文本域中追加消息
     * @param information
     */
    public void writeInfo(String information)
    {
        if (info == null)
        {
            createInfoTextAera();
        }
        this.info.append(information);
    }

    /**
     * 创建消息文本域
     */
    private void createInfoTextAera()
    {
        info = new JTextArea("");
        info.setLineWrap(true);
        info.setRows(12);
        info.setEditable(false);
        info.setFont(new Font("宋体", Font.PLAIN, 15));
        jsp = new JScrollPane(info);
        jsp.setHorizontalScrollBarPolicy(JScrollPane.HORIZONTAL_SCROLLBAR_NEVER);
        jsp.setVerticalScrollBarPolicy(JScrollPane.VERTICAL_SCROLLBAR_AS_NEEDED);
        jsp.setBorder(BorderFactory.createTitledBorder("Info:"));
//        jsp.setFont(new Font("宋体", Font.PLAIN, 14));
        mainPanel.add(jsp, BorderLayout.SOUTH);
    }


    private ServerGUI self = this;
    private JFrame mainFrame = null;
    private JPanel mainPanel = null;
    private JPanel panel1 = null;
    private JPanel panel2 = null;
    private FlowLayout fl1 = null;
    private FlowLayout fl2 = null;
    private BorderLayout bl = null;
    private JButton startButton = null;
    private JButton stopButton = null;
    private JButton exitButton = null;
    private JLabel label1 = null;
    private JTextField listenPort = null;
    private MainAcceptThread server = null;
    private JTextArea info = null;
    private JScrollPane jsp = null;

    private int windowWidth = 520;
    private int windowHeight = 350;


    /**
     * 主方法
     * @param args
     */
    public static void main(String[] args)
    {
        new ServerGUI();
    }
}
