/**
 * 
 */
package mt_tcp.client;

import java.io.BufferedInputStream;
import java.io.BufferedOutputStream;
import java.io.File;
import java.io.IOException;
import java.io.RandomAccessFile;
import java.net.Socket;
import java.net.UnknownHostException;

/**
 * @author hcy
 * 
 */
public class TransferThreadClient implements Runnable
{
	/**
	 * 构造函数
	 * 
	 * @param host
	 *            服务器主机名
	 * @param port
	 *            服务器端口号
	 * @param fileName
	 *            文件名
	 * @param startPos
	 *            传送开始位置
	 * @param Length
	 *            传送长度
	 * @param wrtFile
	 *            写入的文件文件名
	 */
	public TransferThreadClient(String host, int port, String fileName, long startPos, long Length, String wrtFile)
	{
		this.hostname = host;
		this.port = port;
		this.requiredFileName = fileName;
		
		this.startPos = startPos;
		this.startPosStr = String.valueOf(this.startPos);
		this.transferLength = Length;
		this.transferLengthStr = String.valueOf(this.transferLength);
		
		this.writeToFileName = wrtFile;
		
		// 开辟缓存
		buffer = new byte[BUFFERSIZE];
		
	}
	
	@Override
	public void run()
	{
		
		// 创建socket
		createSocket();
		
		// 传送数据
		transfer();
	}
	
	/**
	 * 创建socket
	 */
	private void createSocket()
	{
		try
		{
			// 创建socket
			transferSocket = new Socket(hostname, port);
			// 获得输入输出流
			inFromServer = new BufferedInputStream(transferSocket.getInputStream());
			outToServer = new BufferedOutputStream(transferSocket.getOutputStream());
			
		}
		catch (UnknownHostException e)
		{
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
		catch (IOException e)
		{
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
		
	}
	
	/**
	 * 传送数据
	 */
	private void transfer()
	{
		byte[] temp = null;
		
		try
		{
			// 发送传送数据的请求
			temp = "Transfer".getBytes();
			outToServer.write(temp, 0, temp.length);
			outToServer.flush();
			// 等待确认信息
			inFromServer.read(buffer);
			
			// 发送请求文件的文件名
			temp = requiredFileName.getBytes();
			outToServer.write(temp, 0, temp.length);
			outToServer.flush();
			// 等待确认信息
			inFromServer.read(buffer);
			
			// 发送传输的开始位置，文件读取的开始位置
			temp = startPosStr.getBytes();
			outToServer.write(temp, 0, temp.length);
			outToServer.flush();
			// 等待确认信息
			inFromServer.read(buffer);
			
			// 发送传送的长度
			temp = transferLengthStr.getBytes();
			outToServer.write(temp, 0, temp.length);
			outToServer.flush();
			// 等待确认信息
			inFromServer.read(buffer);
			
			/*
			 * 读取数据并写入文件
			 */
			File file = new File(writeToFileName);
			// 创建文件随机读取流
			raf = new RandomAccessFile(file, "rw");
			
			raf.seek(startPos);
			// 读数据并写入文件
			int len = inFromServer.read(buffer);
			long totalLen = 0;
			while (len != -1)
			{
				totalLen += len;
				raf.write(buffer, 0, len);
				len = inFromServer.read(buffer);
			}
			
			System.out.println("Transfer Over!");
			System.out.println("Total length:"+totalLen);
		}
		catch (IOException e)
		{
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
		finally
		{
			//关闭随机读写流
			try
			{
				if(raf != null)
				{
					raf.close();
				}
			}
			catch (IOException e)
			{
			}
			
			close();
		}
	}
	
	/**
	 * 关闭流和socket
	 */
	private void close()
	{
		try
		{
			inFromServer.close();
			outToServer.close();
			transferSocket.close();
		}
		catch (IOException e)
		{
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
		
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
	private Socket transferSocket = null;
	
	// 开始传送的位置
	private long startPos = -1;
	// 字符串型的开始传送位置
	private String startPosStr = null;
	// 传送的文件的长度
	private long transferLength = 0;
	// 字符串型的传送长度
	private String transferLengthStr = null;
	// 请求传送的文件的文件名
	private String requiredFileName = null;
	
	// 写如的文件名
	private String writeToFileName = null;
	// 文件随机读写流
	private RandomAccessFile raf = null;
}
