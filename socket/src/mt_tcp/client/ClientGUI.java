package mt_tcp.client;

import java.awt.BorderLayout;
import java.awt.FlowLayout;
import java.awt.GridLayout;

import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.io.File;
import javax.swing.BorderFactory;
import javax.swing.JButton;
import javax.swing.JFrame;
import javax.swing.JLabel;
import javax.swing.JPanel;
import javax.swing.JProgressBar;
import javax.swing.JTextField;
 
public class ClientGUI
{

    private JFrame mainFrame = null;
    private JPanel mainPanel = null;
    private JButton transferBtn = null;
    private JTextField transferFileNameField = null;
    private JTextField savedFileNameField = null;
    private JLabel label1 = null;
    private JLabel label2 = null;
    private JProgressBar jpb = null;
    private MClientMain transfer = null;

    public ClientGUI()
    {
        mainFrame = new JFrame("Client");
        mainFrame.setSize(350, 155);
        mainFrame.setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);

        label1 = new JLabel("File Name:");
        label2 = new JLabel("Save to:");
        transferFileNameField = new JTextField("2.jpg");
        //transferFileNameField.setHorizontalAlignment(100);
        savedFileNameField = new JTextField("1.jpg");
        //savedFileNameField.setHorizontalAlignment(100);

        GridLayout bln = new GridLayout(2, 2);
        JPanel nPanel = new JPanel(bln);
        nPanel.add(label1);
        nPanel.add(transferFileNameField);
        nPanel.add(label2);
        nPanel.add(savedFileNameField);


        BorderLayout bl = new BorderLayout();
        mainPanel = new JPanel(bl);
        mainPanel.add(nPanel, BorderLayout.NORTH);


        jpb = new JProgressBar();
        jpb.setBorder(BorderFactory.createTitledBorder("Progress:"));
        jpb.setMinimum(0);
        jpb.setMaximum(100);
        //jpb.setValue(500);
        mainPanel.add(jpb, BorderLayout.SOUTH);



        transferBtn = new JButton("Transfer...");
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
                        transfer = new MClientMain();
                        transfer.setFileName(transferFileNameField.getText());
                        transfer.setWriteToFileName(savedFileNameField.getText());
                        transfer.connect();
                    }
                });
                transferThread.start();

                //实时更新进度条
                Thread processorBarUpdate = new Thread(new Runnable()
                {

                    @Override
                    public void run()
                    {
                        int value = jpb.getMinimum();

                        while (value < jpb.getMaximum())
                        {
                            try
                            {
                                File file = new File(savedFileNameField.getText());
                                long length = transfer.getFileLength() + 1;
                                value = (int) (((float) file.length() / length) * 100);
                                System.out.println(file.length());
                                jpb.setValue(++value);
                                Thread.sleep(10);
                            }
                            catch (InterruptedException ex)
                            {
                            }
                        }
                        jpb.setValue(jpb.getMaximum());
                    }
                });
                processorBarUpdate.start();
            }
        });
        FlowLayout fl = new FlowLayout();
        fl.setAlignment(FlowLayout.CENTER);
        JPanel btnPanel = new JPanel(fl);
        btnPanel.add(transferBtn);
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
}
