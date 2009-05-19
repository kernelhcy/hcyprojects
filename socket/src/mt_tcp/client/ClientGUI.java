package mt_tcp.client;

import java.awt.BorderLayout;
import java.awt.Color;
import java.awt.FlowLayout;
import java.awt.Font;
import java.awt.GridLayout;

import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.io.File;
import java.io.IOException;
import java.net.UnknownHostException;
import javax.swing.BorderFactory;
import javax.swing.JButton;
import javax.swing.JFrame;
import javax.swing.JLabel;
import javax.swing.JOptionPane;
import javax.swing.JPanel;
import javax.swing.JProgressBar;
import javax.swing.JTextField;

public class ClientGUI
{

    public ClientGUI()
    {
        mainFrame = new JFrame("Client");
        mainFrame.setSize(windowWidth, windowHeight);
        mainFrame.setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);

        label1 = new JLabel("File Name:");
        label1.setFont(new Font("Courier New", Font.PLAIN, 16));
        label2 = new JLabel("Save As:");
        label2.setFont(new Font("Courier New", Font.PLAIN, 16));
        transferFileNameField = new JTextField("/home/hcy/test.jpg");
        transferFileNameField.setFont(new Font("Courier New", Font.PLAIN, 16));
        savedFileNameField = new JTextField("/home/hcy/2.jpg");
        savedFileNameField.setFont(new Font("Courier New", Font.PLAIN, 16));

        label3 = new JLabel("Host Name:");
        label4 = new JLabel("Port: ");
        label3.setFont(new Font("Courier New", Font.PLAIN, 16));
        label4.setFont(new Font("Courier New", Font.PLAIN, 16));
        hostNameField = new JTextField("192.168.0.76");
        portField = new JTextField("1234");
        hostNameField.setFont(new Font("Courier New", Font.PLAIN, 16));
        portField.setFont(new Font("Courier New", Font.PLAIN, 16));

        GridLayout bln = new GridLayout(4, 2);
        JPanel nPanel = new JPanel(bln);
        nPanel.add(label1);
        nPanel.add(transferFileNameField);
        nPanel.add(label2);
        nPanel.add(savedFileNameField);
        nPanel.add(label3);
        nPanel.add(hostNameField);
        nPanel.add(label4);
        nPanel.add(portField);

        BorderLayout bl = new BorderLayout();
        mainPanel = new JPanel(bl);
        mainPanel.add(nPanel, BorderLayout.NORTH);


        jpb = new JProgressBar();
        jpb.setBorder(BorderFactory.createTitledBorder("Progress:"));
        jpb.setMinimum(0);
        jpb.setMaximum(100);
        //jpb.setValue(500);
        mainPanel.add(jpb, BorderLayout.SOUTH);

        FlowLayout fl = new FlowLayout();
        fl.setAlignment(FlowLayout.CENTER);
        JPanel btnPanel = new JPanel(fl);

        label5 = new JLabel("Thread Number:");
        label5.setFont(new Font("Courier New", Font.PLAIN, 16));
        numberThreadsFiled = new JTextField("1");
        numberThreadsFiled.setColumns(2);
        numberThreadsFiled.setToolTipText("At most 100 threads.");
        numberThreadsFiled.setFont(new Font("Courier New", Font.PLAIN, 16));
        btnPanel.add(label5);
        btnPanel.add(numberThreadsFiled);


        //空白
        btnPanel.add(new JLabel("              "));

        transferBtn = new JButton("Transfer...");
        transferBtn.setFont(new Font("Courier New", Font.PLAIN, 14));
        transferBtn.addActionListener(new ActionListener()
        {

            @Override
            public void actionPerformed(ActionEvent e)
            {
                Thread transferThread = new Thread(new Runnable()
                {

                    public void run()
                    {
                        jpb.setValue(jpb.getMaximum());

                        //恢复所有输入框的背景色
                        transferFileNameField.setBackground(Color.WHITE);
                        savedFileNameField.setBackground(Color.WHITE);
                        hostNameField.setBackground(Color.WHITE);
                        portField.setBackground(Color.WHITE);
                        numberThreadsFiled.setBackground(Color.WHITE);

                        String hostName = hostNameField.getText();
                        int port = -1;
                        try
                        {
                            port = Integer.parseInt(portField.getText());
                        }
                        catch (NumberFormatException ee)
                        {
                            showWarnErrorMsg("Bad Port Number! " + ee.getMessage());
                            portField.setBackground(Color.RED);
                            return;
                        }
                        if (port < 1000)
                        {
                            showWarnErrorMsg("Bad Port Number! ");
                            portField.setBackground(Color.RED);
                            return;
                        }


                        int threadNumber = -1;
                        try
                        {
                            threadNumber = Integer.parseInt(numberThreadsFiled.getText());
                        }
                        catch (NumberFormatException ee)
                        {
                            showWarnErrorMsg("Bad Thread Number! " + ee.getMessage());
                            numberThreadsFiled.setBackground(Color.RED);
                            return;
                        }
                        if (threadNumber > 100 || threadNumber < 0)
                        {
                            showWarnErrorMsg("Bad Thread Number! ");
                            numberThreadsFiled.setBackground(Color.RED);
                            return;
                        }

                        transfer = new MClientMain(hostName, port);
                        transfer.setFileName(transferFileNameField.getText());
                        transfer.setWriteToFileName(savedFileNameField.getText());
                        try
                        {
                            //用于测试线程，测试是否刻是传送
                            isTransferring = true;
                            int state = transfer.connect();
                            if (state == MClientMain.FILE_NOT_EXIST)
                            {
                                showWarnErrorMsg("The file is not exist!");
                                transferFileNameField.setBackground(Color.RED);
                                transfer.close();
                                return;
                            }

                        }
                        catch (UnknownHostException ex)
                        {
                            showWarnErrorMsg("Unknown Host! " + ex.getMessage());
                            hostNameField.setBackground(Color.RED);
                            return;
                        }
                        catch (IOException ex)
                        {
                            showWarnErrorMsg("Error!" + ex.getMessage());
                            return;
                        }
                        finally
                        {
                            transfer.close();
                        }
                    }
                });
                transferThread.start();

                //实时更新进度条
                Thread processorBarUpdate = new Thread(new Runnable()
                {

                    @Override
                    public void run()
                    {
                        if (!isTransferring)
                        {
                            return;
                        }
                        int value = jpb.getMinimum();

                        while (value < jpb.getMaximum())
                        {
                            try
                            {
                                File file = new File(savedFileNameField.getText());
                                long length = transfer.getFileLength() + 1;
                                value = (int) (((float) file.length() / length) * 100);
                                //System.out.println(file.length());
                                jpb.setValue(++value);
                                Thread.sleep(10);
                            }
                            catch (Exception ex)
                            {
                                ;
                            }
                        }
                        jpb.setValue(jpb.getMaximum());

                        //文件传送完毕
                        JOptionPane.showMessageDialog(mainFrame, "File transfer over!");
                        isTransferring = false;
                    }
                });
                processorBarUpdate.start();
            }

            /**
             *   提示错误和警告信息
             */
            private void showWarnErrorMsg(String msg)
            {
                System.out.println(msg);
                JOptionPane.showMessageDialog(mainFrame, msg, "Warning",
                    JOptionPane.WARNING_MESSAGE, null);
            }
        });

        btnPanel.add(transferBtn);

        //get file list
        getFilelist = new JButton("Get Filelist");
        getFilelist.setFont(new Font("Courier New", Font.PLAIN, 14));
        getFilelist.addActionListener(new ActionListener()
        {

            @Override
            public void actionPerformed(ActionEvent e)
            {
                System.out.println("Get file list");
                Thread getList = new Thread(new GetFilelist("localhost", 1234, "/home/hcy/workspace"));

                getList.start();
            }
        });


        btnPanel.add(getFilelist);
        mainPanel.add(btnPanel, BorderLayout.CENTER);



        mainPanel.setBorder(BorderFactory.createTitledBorder(""));

        mainFrame.getContentPane().add(mainPanel);
        mainFrame.setVisible(true);
    }

    /**
     * @param args
     */
    public static void main(String[] args)
    {
        new ClientGUI();
    }
    private JFrame mainFrame = null;
    private JPanel mainPanel = null;
    private JButton transferBtn = null;
    private JTextField transferFileNameField = null;
    private JTextField savedFileNameField = null;
    private JTextField hostNameField = null;
    private JTextField portField = null;
    private JLabel label1 = null;
    private JLabel label2 = null;
    private JLabel label3 = null;
    private JLabel label4 = null;
    private JProgressBar jpb = null;
    private MClientMain transfer = null;
    private JLabel label5 = null;
    private JTextField numberThreadsFiled = null;
    private volatile boolean isTransferring = false;
    private JButton getFilelist = null;
    private int windowWidth = 350;
    private int windowHeight = 240;
}
