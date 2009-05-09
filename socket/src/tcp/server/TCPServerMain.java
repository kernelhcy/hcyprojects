package tcp.server;

import java.io.BufferedInputStream;
import java.io.BufferedOutputStream;
import java.io.BufferedReader;
import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;
import java.io.InputStreamReader;
import java.net.ServerSocket;
import java.net.Socket;

/**
 * 使用TCP协议进行传输的服务器端
 * *****************************************************************
 * 要使用缓冲流（BufferedOutputstream等），否则会报connect reset异常！！！
 * *****************************************************************
 * @author hcy
 * 
 */
public class TCPServerMain
{
	private static final int BUFFERSIZE = 1024;
	// 客户端发送的数据
	private String input = null;
	// 返回给客户端的数据
	private byte[] result = null;
	// 服务socket
	private ServerSocket serverSocket = null;
	// 连接socket
	private Socket connetcionSocket = null;
	// 输入数据流
	private BufferedReader inFromClient = null;
	// 输出数据流
	private BufferedOutputStream outTOClient = null;
	// socket监听的端口号
	private int port;
	
	// 记录处理的请求的个数
	private static int cnt = 0;
	
	/**
	 * 默认监听的端口号为1234
	 */
	public TCPServerMain()
	{
		this(1234);
	}
	
	/**
	 * 设置监听的端口号
	 * 
	 * @param port
	 */
	public TCPServerMain(int port)
	{
		this.port = port;
		
		try
		{
			// 创建serversocket
			System.out.println("创建ServerSocket...");
			serverSocket = new ServerSocket(this.port);
			
			/*
			 * 设置连接超时。不限时。
			 */
			serverSocket.setSoTimeout(0);
			
		}
		catch (IOException e)
		{
			// TODO Auto-generated catch block
			System.out.println("创建ServerSocket出错...");
			e.printStackTrace();
		}
	}
	
	public void acceptConnection()
	{
		if (serverSocket == null)
		{
			System.out.println("服务器启动失败...");
			return;
		}
		System.out.println("开始接受请求...");
		BufferedInputStream dis = null;
		while (true)
		{
			
			try
			{
				// 接受请求cd
				connetcionSocket = serverSocket.accept();
				
				/*
				 * 设置连接超时。不限时。
				 */
				connetcionSocket.setSoTimeout(0);
				
				System.out.println("接受第" + (++cnt) + "个请求...");
				System.out.println("主机地址:" + connetcionSocket.getInetAddress().getHostAddress());
				System.out.println("端口：" + connetcionSocket.getPort());
				
				// 获得输入输出流
				inFromClient = new BufferedReader(new InputStreamReader(connetcionSocket.getInputStream()));
				outTOClient = new BufferedOutputStream(connetcionSocket.getOutputStream());
				
				input = inFromClient.readLine();
				System.out.println("请求数据：" + input);
				
				//从文件1.jpg中读取数据
				File file = new File("生命之鹰.jpg");
				dis = new BufferedInputStream(new FileInputStream(file));
				result = new byte[BUFFERSIZE];
				System.out.println("输出数据...");
				
				int len = 0;//读取到缓冲区中的字节数
				len = dis.read(result);
				while (len != -1)
				{
					/**
					 * 必须按照缓冲区中实际的数据量写入流中，不可用outTOClient.write(result);简单地
					 * 将缓冲区统统写到流中，否则会报connection reset异常！！！
					 */
					outTOClient.write(result,0,len);
					//outTOClient.flush();
					len = dis.read(result);
					//System.out.println(len);
				}
				
				outTOClient.flush();
				
				System.out.println("文件总长度："+file.length()+"b.");
				
			}
			catch (IOException e)
			{
				// TODO Auto-generated catch block
				System.out.println("创建连接socket失败...");
				e.printStackTrace();
			}
			finally
			{
				try
				{
					//关闭流和socket
					dis.close();
					inFromClient.close();
					outTOClient.close();
					connetcionSocket.close();
				}
				catch (IOException e)
				{
					// TODO Auto-generated catch block
					e.printStackTrace();
				}
				
				System.out.println("请求处理结束。");
			}
			
			//接收1次请求
			if (cnt >= 1000)
			{
				break;
			}
		}
		
		try
		{
			serverSocket.close();
		}
		catch (IOException e)
		{
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
		return;
	}
	
	/**
	 * @param args
	 */
	public static void main(String[] args)
	{
		// TODO Auto-generated method stub
		new TCPServerMain(1234).acceptConnection();
	}
	
}
