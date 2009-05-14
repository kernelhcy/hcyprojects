package tcp.client;

import java.io.BufferedInputStream;
import java.io.BufferedOutputStream;
import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.net.Socket;
import java.net.UnknownHostException;

/**
 * 使用TCP协议进行传输的客户端
 * 
 * *****************************************************************
 * 要使用缓冲流（BufferedOutputstream等），否则会报connect reset异常！！！
 * *****************************************************************
 * 
 * @author hcy
 * 
 */
public class TCPClientMain
{
	private static final int BUFFERSIZE = 1024;
	// 主机名
	private String hostname = null;
	// 端口号
	private int port;
	// 输出到服务器的信息
	private String data = null;
	// 服务器返回的信息
	private byte[] result = null;
	// 向服务器的输出流
	private BufferedOutputStream outToServer = null;
	// 从服务器的输入流
	private BufferedInputStream inFromServer = null;
	// 连接socket
	private Socket clientSocket = null;
	// 接受到的文件的长度
	private int length = 0;
	
	/**
	 * 默认的服务器主机名为localhost,端口为1234
	 */
	public TCPClientMain()
	{
		this("localhost", 1234);
	}
	
	/**
	 * 设定服务器主机名和端口号
	 * 
	 * @param hostname
	 * @param port
	 */
	public TCPClientMain(String hostname, int port)
	{
		this.hostname = hostname;
		this.port = port;
	}
	
	public void connect()
	{
		//用于计算传送时间
		long startTime = 0;//开始接收文件的时间
		long endTime = 0;//接收完毕是的时间
		
		//文件输出流，将收到的数据写入文件
		BufferedOutputStream tofile = null;
		try
		{
			System.out.println("创建socket连接.");
			// 建立socket
			clientSocket = new Socket(hostname, port);
			
			// socket连接不限时
			clientSocket.setSoTimeout(0);
			
			System.out.println("主机名：" + hostname);
			System.out.println("端口号：" + port);
			
			// 获得输出流
			outToServer = new BufferedOutputStream(clientSocket.getOutputStream());
			// 获得输入流
			inFromServer = new BufferedInputStream(clientSocket.getInputStream());
			
			data = "Hello!This is client...\nGood night.\n";
			
			System.out.println("发送数据.");
			// 向服务器发送数据
			outToServer.write(data.getBytes());
			outToServer.flush();
			
			result = new byte[BUFFERSIZE];
			
			System.out.println("接受数据.");
			//创建文件2.jgp,用于存储接受到的数据
			File file = new File("2.jpg");
			file.canWrite();//可写
			file.delete();//删除已经存在的同名文件
			//创建文件
			if (!file.createNewFile())
			{
				System.out.println("创建文件失败...");
				return;
			}
			tofile = new BufferedOutputStream(new FileOutputStream(file));
			
			startTime = System.currentTimeMillis();
			
			int temp;//从输入流中读取的字节数
			temp = inFromServer.read(result);
			while (temp != -1)
			{
				length += temp;
				//显示进度
				if (length % (BUFFERSIZE * 10000) == 0)
				{
					System.out.print(".");
				}
				
				//写入文件
				//tofile.write(result, 0, temp);
				//tofile.flush();
				
				temp = inFromServer.read(result);
				//System.out.println(temp);
			}
			
			//tofile.flush();
			
		}
		catch (UnknownHostException e)
		{
			// TODO Auto-generated catch block
			System.out.println("未知的主机名...");
			e.printStackTrace();
		}
		catch (IOException e)
		{
			// TODO Auto-generated catch block
			e.printStackTrace();
			System.out.println("IO错误!!");
			
		}
		finally
		{
			try
			{
                endTime = System.currentTimeMillis();
				tofile.close();
				System.out.println("总用时： " + (endTime - startTime) + " ms.");
				System.out.println("接受到文件大小： " + length  + " b.");
				// System.out.println("平均传送速率： "+(length/1024/1024)/((long)(endTime-startTime)/1000)+" Mb/s");
				System.out.println("关闭连接.");
				
				inFromServer.close();
				outToServer.close();
				// 关闭socket
				clientSocket.close();
			}
			catch (IOException e)
			{
				// TODO Auto-generated catch block
				e.printStackTrace();
			}
			
		}
		
	}
	
	public String getHostname()
	{
		return hostname;
	}
	
	public void setHostname(String hostname)
	{
		this.hostname = hostname;
	}
	
	public int getPort()
	{
		return port;
	}
	
	public void setPort(int port)
	{
		this.port = port;
	}
	
	public String getData()
	{
		return data;
	}
	
	public void setData(String data)
	{
		this.data = data;
	}
	
	/**
	 * @param args
	 */
	public static void main(String[] args)
	{
		// TODO Auto-generated method stub
		new TCPClientMain("192.168.0.76", 1234).connect();
	}
	
}
