package udp.server;

import java.io.BufferedInputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;
import java.net.DatagramPacket;
import java.net.DatagramSocket;
import java.net.InetAddress;
import java.net.SocketException;
import java.util.logging.Level;
import java.util.logging.Logger;

/**
 * 使用UDP协议进行传输的服务器端 
 * @author hcy
 *
 */
public class UDPServerMain
{
    //缓冲区大小

    private static final int BUFFERSIZE = 10240;
    //服务器socket
    private DatagramSocket serverSocket = null;
    //接收数据缓冲区
    private byte[] receiveData = null;
    //发送数据缓冲区
    private byte[] sendData = null;
    //发送数据的客户端的IP地址
    private InetAddress clientIPAddress = null;
    //发送数据的客户端的端口号
    private int clientPort = 0;
    //监听的端口号
    private int listenedPort = 0;
    //接收数据的数据报
    private DatagramPacket receivePacket = null;
    //发送数据的数据报
    private DatagramPacket sendPacket = null;

    /**
     * 使用默认端口号1234
     */
    public UDPServerMain()
    {
        this(1234);
    }

    /**
     * 创建服务器，使用端口号port
     * @param port
     */
    public UDPServerMain(int port)
    {
        this.listenedPort = port;
        try
        {
            System.out.println("创建DatagramSocket.");
            serverSocket = new DatagramSocket(listenedPort);
            System.out.println("监听端口号： " + this.listenedPort);

            /*
             * 设置连接超时不受限制。
             * 可防止传送大数据时出现连接重置错误。
             */
            serverSocket.setSoTimeout(0);
            //设置发送接收缓冲区大小
            serverSocket.setReceiveBufferSize(BUFFERSIZE);
            serverSocket.setSendBufferSize(BUFFERSIZE);

            //创建缓冲区
            System.out.println("创建缓冲区.");
            sendData = new byte[BUFFERSIZE];
            receiveData = new byte[BUFFERSIZE];
        }
        catch (SocketException e)
        {
            // TODO Auto-generated catch block
            System.out.println("创建DatagramSocket失败。");
            e.printStackTrace();
        }
    }

    void run()
    {
        //while(true)
        {
            this.acceptConnect();
        }
        //serverSocket.close();
    }

    /**
     * 处理客户端请求
     */
    void acceptConnect()
    {
        receivePacket = new DatagramPacket(receiveData, receiveData.length);
        try
        {
            //接收来自客户端的数据并显示
            System.out.println("接收来自客户端的数据.");
            serverSocket.receive(receivePacket);
            String msg = new String(receivePacket.getData());
            System.out.println("客户端的消息： " + msg);

            //客户端的信息
            clientIPAddress = receivePacket.getAddress();
            clientPort = receivePacket.getPort();
            System.out.println("客户端的IP地址为： " + clientIPAddress.getHostAddress());
            System.out.println("客户端的端口号： " + clientPort);

            //创建发送数据的数据报
            sendPacket = new DatagramPacket(sendData, sendData.length, clientIPAddress, clientPort);

            //发送数据
            System.out.println("发送数据...");
            File file = new File("/home/hcy/test.jpg");
            BufferedInputStream dis = new BufferedInputStream(new FileInputStream(file));
            int cnt = dis.read(sendData);
            while (cnt != -1)
            {
                serverSocket.send(sendPacket);
                cnt = dis.read(sendData);
                try
                {
                    Thread.sleep(2);
                }
                catch (InterruptedException ex)
                {
                    Logger.getLogger(UDPServerMain.class.getName()).log(Level.SEVERE, null, ex);
                }

            }
            dis.close();
            System.out.println("发送完毕.");

            //关闭服务器socket
            //serverSocket.close();
        }
        catch (IOException e)
        {
            // TODO Auto-generated catch block
            System.out.println("IO错误");
            e.printStackTrace();
        }
        finally
        {
            //关闭服务器socket
            serverSocket.close();

        }
    }

    /**
     * @param args
     */
    public static void main(String[] args)
    {
        // TODO Auto-generated method stub
        new UDPServerMain(1234).run();
    }
}
