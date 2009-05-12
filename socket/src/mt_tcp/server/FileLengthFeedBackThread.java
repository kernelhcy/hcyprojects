package mt_tcp.server;

import java.io.BufferedInputStream;
import java.io.BufferedOutputStream;
import java.io.File;
import java.io.IOException;
import java.net.Socket;
import java.net.SocketException;

/**
 * 用于返回客户端其请求的文件的长度。
 * 
 * @author hcy
 *
 */
public class FileLengthFeedBackThread implements Runnable
{
	/**
	 * 
	 * @param conn
	 */
	public FileLengthFeedBackThread(Socket conn)
	{
		this.connetcionSocket = conn;
	}

	/**
	 * 
	 */
	@Override
	public void run()
	{
		/*
		 * 设置连接超时。不限时。
		 */
		try
		{
			buffer = new byte[BUFFERSIZE];
			
			connetcionSocket.setSoTimeout(0);
			
			// 获得输入输出流
			inFromClient = new BufferedInputStream(connetcionSocket.getInputStream());
			outToClient = new BufferedOutputStream(connetcionSocket.getOutputStream());
			
			long length = -1;//文件长度 bytes
			
			/*
			 * 读取客户端请求的文件的文件名
			 * 注：
			 * 		此处假设文件名长度不超过1024bytes!!! 
			 */
			int fileNameLength = inFromClient.read(buffer);
			if(fileNameLength != -1)
			{
				input = new String(buffer,0,fileNameLength);
				System.out.println("Requested file name： "+input);
				File file = new File(input);
				if(file.exists())
                {
                    length = file.length();
                }
                else
                {
                    length = -1;
                }
			}
			
			/*
			 * 返回客户端所请求的文件的长度
			 * 当无法获得文件的长度，如文件不存在，时，返回文件长度为-1
			 */
			
			//转换成字符串
			String stringlen = String.valueOf(length); 
			//发送
			outToClient.write(stringlen.getBytes(),0,stringlen.getBytes().length);
			outToClient.flush();
			
			/*
			 * 确认客户端收到了文件长度信息。
			 * 防止过早的发送文件数据。
			 */
			inFromClient.read(buffer);
			
		}
		catch (SocketException e)
		{
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
		catch (IOException e)
		{
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
		finally
		{
			try
			{
				// 关闭流和socket
				inFromClient.close();
				outToClient.close();
				connetcionSocket.close();
			}
			catch (IOException e)
			{
				// TODO Auto-generated catch block
				e.printStackTrace();
			}
		}
		
	}
	
	
	/*
	 * ******************************* 
	 *        声明成员变量
	 * *******************************
	 */
	


	// 缓存大小
	private static final int BUFFERSIZE = 1024;
	// 客户端发送的数据
	private String input = null;
	// 返回给客户端的数据
	private byte[] buffer = null;
	// 连接socket
	private Socket connetcionSocket = null;
	// 输入数据流
	private BufferedInputStream inFromClient = null;
	// 输出数据流
	private BufferedOutputStream outToClient = null;
	
}
