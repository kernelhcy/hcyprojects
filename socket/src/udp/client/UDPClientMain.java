package udp.client;

import java.io.BufferedOutputStream;
import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.net.DatagramPacket;
import java.net.DatagramSocket;
import java.net.InetAddress;
import java.net.SocketException;
import java.net.UnknownHostException;



/**
 * 使用UDP协议进行传输的客户端
 * @author hcy
 *
 */
public class UDPClientMain
{
	//缓冲区大小
	private static final int BUFFERSIZE = 10240;
	//向服务器发送的信息
	private String sendMsg = null;
	//客户端socket
	private DatagramSocket clientSocket = null;
	
	//服务器IP地址
	private InetAddress serverIPAddress = null;
	//服务器端口号
	private int severPort = 0;
	
	//发送缓冲区
	private byte[] sendData = null;
	//接收缓冲区
	private byte[] receiveData = null;
	
	//发送数据的数据报
	private DatagramPacket sendPacket = null;
	//接收数据的数据报
	private DatagramPacket receivePacket = null;
	
	/**
	 * 使用默认的主机名和端口号
	 * localhost 1234
	 */
	public UDPClientMain()
	{
		this("localhost",1234);
	}
	
	/**
	 * 设置主机名和端口号
	 * @param hostname
	 * @param port
	 */
	public UDPClientMain(String hostname,int port)
	{
		try
		{
			System.out.println("创建socket.");
			clientSocket = new DatagramSocket();
			/*
			 * 设置连接超时
			 */
			clientSocket.setSoTimeout(1000);
			//设置发送接收缓冲区大小
			clientSocket.setReceiveBufferSize(BUFFERSIZE);
			clientSocket.setSendBufferSize(BUFFERSIZE);
			
			System.out.println("获取服务器的IP地址");
			serverIPAddress = InetAddress.getByName(hostname);
			this.severPort = port;
			
			
			//创建缓冲区
			System.out.println("创建缓冲区.");
			//sendData = new byte[BUFFERSIZE];
			receiveData = new byte[BUFFERSIZE];
			
			sendMsg = "Hello.I want a picture.";
			
		}
		catch (SocketException e)
		{
			// TODO Auto-generated catch block
			System.out.println("创建socket失败.");
			e.printStackTrace();
		}
		catch (UnknownHostException e)
		{
			// TODO Auto-generated catch block
			System.err.println("未知的主机名.");
			e.printStackTrace();
		}
		
	}
	
	/**
	 * 
	 */
	void connect()
	{
		System.out.println("发送数据.");
		sendData = sendMsg.getBytes();
		sendPacket = new DatagramPacket(sendData,sendData.length,serverIPAddress,severPort);
		try
		{
			clientSocket.send(sendPacket);
		}
		catch (IOException e)
		{
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
		
		System.out.println("接收数据.");
		receivePacket = new DatagramPacket(receiveData,receiveData.length);
		
		File file = new File("/home/hcy/2.jpg");
		
		try
		{
			System.out.println("文件操作.");
			file.delete();
			file.createNewFile();
			file.canWrite();
		}
		catch (IOException e)
		{
			// TODO Auto-generated catch block
			System.err.println("文件操作IO错误.");
			e.printStackTrace();
		}
		
		System.out.println("接收数据.");
		int length = 0;//接收的文件的长度
		BufferedOutputStream dos = null;
		
		long beginTime = System.currentTimeMillis();
		
		try
		{
			//创建文件输出流
			dos = new BufferedOutputStream(new FileOutputStream(file));
			while(true)
			{
				
				//接收数据
				clientSocket.receive(receivePacket);
				
				length += receivePacket.getLength();
				//将数据写入文件
				dos.write(receiveData,0,receivePacket.getLength());
				//System.out.println(receiveData);
			}
		
		}
		catch (FileNotFoundException e)
		{
			// TODO Auto-generated catch block
			System.err.println("没有找到文件"+e.getMessage());
			e.printStackTrace();
		}
		catch (IOException e)
		{
			// TODO Auto-generated catch block
			System.err.println("接收超时.");
			
			
		}
		finally
		{
			try
			{
				if(dos != null)
				{
					dos.write(receiveData);
					dos.flush();
					dos.close();
				}
			}
			catch (IOException e1)
			{
				// TODO Auto-generated catch block
				e1.printStackTrace();
			}
			
			long endTime = System.currentTimeMillis();
			
			System.out.println("数据接收完毕.");
			System.out.println("接收到的数据长度: "+length+" b.");
			System.out.println("总共耗时： "+(endTime-beginTime)+"ms");
			//关闭socket
			clientSocket.close();
		}
	}
	
	/**
	 * @param args
	 */
	public static void main(String[] args)
	{
		// TODO Auto-generated method stub
		new UDPClientMain().connect();
	}
	
}
