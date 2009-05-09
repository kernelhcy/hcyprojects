package mt_tcp.client;

import java.io.BufferedInputStream;
import java.io.BufferedOutputStream;
import java.io.File;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.net.Socket;
import java.net.SocketException;
import java.net.UnknownHostException;

public class MClientMain
{
	
	/**
	 * 默认的服务器主机名为localhost,端口为1234
	 */
	public MClientMain()
	{
		this("localhost", 1234);
	}
	
	/**
	 * 设定服务器主机名和端口号
	 * 
	 * @param hostname
	 * @param port
	 */
	public MClientMain(String hostname, int port)
	{
		this.hostname = hostname;
		this.port = port;
		// 测试用
		this.requiredFileName = "2.jpg";
		this.writeToFileName = "1.jpg";
	}
	
	public void connect()
	{
		
		try
		{
			System.out.println("创建socket,输入流和输出流");
			// 创建socket,输入流和输出流
			createSocket();
			
			// 开辟缓存
			buffer = new byte[BUFFERSIZE];
			
			System.out.println("读取文件的长度");
			// 读取文件的长度
			getFileLengthFromServer();
			
			System.out.println("接收数据...");
			// 接收数据
			long beginTime = System.currentTimeMillis();
			receiveDataFromServer();
			if (done)
			{
				long endTime = System.currentTimeMillis();
				System.out.println("总用时：" + (endTime - beginTime)/1000+"s.");
			}
			
		}
		catch (UnknownHostException e)
		{
			System.out.println("未知的主机名...");
			e.printStackTrace();
		}
		catch (IOException e)
		{
			e.printStackTrace();
			System.out.println("IO错误!!");
		}
		finally
		{
			// 关闭连接，释放资源
			close();
		}
		
	}
	
	/**
	 * 关闭输入输出流和socket。
	 */
	private void close()
	{
		try
		{
			
			inFromServer.close();
			outToServer.close();
			// 关闭socket
			clientSocket.close();
		}
		catch (IOException e)
		{
			System.out.println("关闭资源错误..");
			e.printStackTrace();
		}
	}
	
	/**
	 * 接收数据并写入文件
	 * 
	 * @param writeToFileName
	 *            写入的文件的文件名
	 * @throws IOException
	 * @throws FileNotFoundException
	 */
	private void receiveDataFromServer()
	{
		File file = new File(writeToFileName);
		if(file.exists())
		{
			file.delete();
		}
		try
		{
			file.createNewFile();
		}
		catch (IOException e1)
		{
		}
		
		
		if (fileLength > 10240)// 大于10k，多线程传送
		{
			long perLength = fileLength / (long) numberOfThreads;
			threads = new Thread[numberOfThreads + 1];
			long tempstart = 0, remainLen = fileLength;// 剩余的文件长度
			int i = 0;
			while (i < numberOfThreads && remainLen > perLength)
			{
				System.out.println("创建线程"+(i+1));
				threads[i] = new Thread(new TransferThreadClient(hostname, port, requiredFileName, tempstart,
						perLength, writeToFileName));
				threads[i].start();
				tempstart += perLength;
				remainLen -= perLength;
				++i;
			}
			if (remainLen > 0)
			{
				System.out.println("创建线程"+(i+1));
				threads[i] = new Thread(new TransferThreadClient(hostname, port, requiredFileName, tempstart,
						remainLen, writeToFileName));
				threads[i].start();
			}
			
		}
		else
		// 当传送的文件长度小于10k时，只用一个线程传送
		{
			new Thread(new TransferThreadClient(hostname, port, requiredFileName, 0, fileLength, writeToFileName))
					.start();
		}
		
		//检查是否已经传送完毕
		Thread acks = new Thread(new Acks());
		acks.start();
		try
		{
			acks.join();
		}
		catch (InterruptedException e)
		{
			e.printStackTrace();
		}
	}
	
	/**
	 * 创建socket 创建输入和输出流
	 * 
	 * @throws UnknownHostException
	 * @throws IOException
	 * @throws SocketException
	 */
	private void createSocket() throws UnknownHostException, IOException, SocketException
	{
		System.out.println("创建socket连接.");
		// 建立socket
		clientSocket = new Socket(hostname, port);
		
		// socket连接不限时
		clientSocket.setSoTimeout(0);
		
		// 获得输出流
		outToServer = new BufferedOutputStream(clientSocket.getOutputStream());
		// 获得输入流
		inFromServer = new BufferedInputStream(clientSocket.getInputStream());
	}
	
	/**
	 * 返回向服务器请求的文件的文件名
	 * 
	 * @return the fileName
	 */
	public String getFileName()
	{
		return requiredFileName;
	}
	
	/**
	 * 设置请求的文件的文件名
	 * 
	 * @param fileName
	 *            the fileName to set
	 */
	public void setFileName(String fileName)
	{
		this.requiredFileName = fileName;
	}
	
	/**
	 * 从服务器读取文件的长度
	 * 
	 * @throws IOException
	 */
	private void getFileLengthFromServer() throws IOException
	{
		String info = "GetFileLength";
		/*
		 * 请求服务器发送文件长度
		 */
		outToServer.write(info.getBytes(), 0, info.getBytes().length);
		outToServer.flush();
		// 确认服务器收到请求
		inFromServer.read(buffer);
		
		/*
		 * 向服务器发送请求传送的文件的文件名
		 */
		outToServer.write(requiredFileName.getBytes(), 0, requiredFileName.getBytes().length);
		outToServer.flush();
		
		/*
		 * 获取文件的长度
		 */
		int lenLen = -1;// 从服务器读取的数据的长度
		lenLen = inFromServer.read(buffer);
		fileLength = Long.parseLong(new String(buffer, 0, lenLen));
		System.out.printf("文件的长度为：%d bytes\n", fileLength);
		
		/*
		 * 向服务器发送确认收到文件长度。 防止服务器过早的发送文件数据。
		 */
		outToServer.write("Got length".getBytes());
		outToServer.flush();
	}
	
	/**
	 * 轮询所有传送线程，检查是否都已经传送完毕
	 * @author hcy
	 *
	 */
	private class Acks implements Runnable
	{
		
		@Override
		public void run()
		{
			boolean allStop = false;
			while (!allStop)
			{
				allStop = true;
				for (int i = 0; i < threads.length; ++i)
				{
					if(threads[i] != null && threads[i].isAlive())
					{
						allStop = false;
					}
					
				}
				
				
				try
				{
					Thread.sleep(500);
				}
				catch (InterruptedException e)
				{
					// TODO Auto-generated catch block
					e.printStackTrace();
				}
			}
			
			done = true;
			
		}
		
	}
	
	/**
	 * 返回所连接的服务器的主机名
	 * 
	 * @return
	 */
	public String getHostname()
	{
		return hostname;
	}
	
	/**
	 * 设置所连接的服务器主机名
	 * 
	 * @param hostname
	 */
	public void setHostname(String hostname)
	{
		this.hostname = hostname;
	}
	
	/**
	 * 返回所连接的服务器端口号
	 * 
	 * @return
	 */
	public int getPort()
	{
		return port;
	}
	
	/**
	 * 设置所连接的服务器端口号
	 * 
	 * @param port
	 */
	public void setPort(int port)
	{
		this.port = port;
	}
	
	/**
	 * @return the numberOfThreads
	 */
	public int getNumberOfThreads()
	{
		return numberOfThreads;
	}
	
	/**
	 * @param numberOfThreads
	 *            the numberOfThreads to set
	 */
	public void setNumberOfThreads(int numberOfThreads)
	{
		this.numberOfThreads = numberOfThreads;
	}

    /**
     * 获得传送的文件的长度。
     * @return
     */
    public long getFileLength()
    {
        return fileLength;
    }

    /**
     * 
     * @return
     */
    public String getWriteToFileName()
    {
        return this.writeToFileName;
    }

    /**
     *
     * @param name
     */
    public void setWriteToFileName(String name)
    {
        this.writeToFileName = name;
    }
	/*
	 * ***************************** 声明成员变量 *****************************
	 */

	private static final int BUFFERSIZE = 1024;
	// 主机名
	private String hostname = null;
	// 端口号
	private int port;
	
	// 服务器返回的信息
	private byte[] buffer = null;
	// 向服务器的输出流
	private BufferedOutputStream outToServer = null;
	// 从服务器的输入流
	private BufferedInputStream inFromServer = null;
	// 连接socket
	private Socket clientSocket = null;
	
	// 传送的文件的长度
	private long fileLength = 0;
	// 请求传送的文件的文件名
	private String requiredFileName = null;
	
	// 传送数据的线程数
	// 默认为五个线程
	private int numberOfThreads = 1;
	// 传送的线程
	private volatile Thread[] threads = null;
	//标记传送完毕
	private boolean done = false;
	
	// 写如的文件名
	private String writeToFileName = null;
	
	/**
	 * @param args
	 */
//	public static void main(String[] args)
//	{
//		new MClientMain("localhost", 1234).connect();
//
//	}
	
}
