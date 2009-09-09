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
import mt_tcp.server.MyLogger;
import mt_tcp.server.TransferLengthInfo;

/**
 * @author hcy
 * 
 */
public class TransferThreadClient implements Runnable
{

	/**
	 * 构造函数
	 * 
	 */
	public TransferThreadClient(Builder builder)
	{
		this.hostname = builder.hostname;
		this.port = builder.port;
		this.requiredFileName = builder.requiredFileName;
		this.startPos = builder.startPos;
		this.startPosStr = builder.startPosStr;
		//this.transferLength = builder.transferLength;
		this.transferLengthStr = builder.transferLengthStr;
		this.writeToFileName = builder.writeToFileName;
		this.transferLengthInfo = builder.transferLengthInfo;

		// 开辟缓存
		buffer = new byte[BUFFERSIZE];

		logger = MyLogger.getInstance();

	}

	/**
	 * 建造者模式 建造者
	 */
	public static class Builder
	{
		private String hostname = null;
		private int port;
		private String requiredFileName = null;
		private long startPos;
		private String startPosStr = null;
		private long transferLength;
		private String transferLengthStr = null;
		private String writeToFileName = null;
		private TransferLengthInfo transferLengthInfo = null;

		public Builder()
		{
		}

		/*
		 * 设置参数
		 * 
		 * @param host 服务器主机名
		 * 
		 * @param port 服务器端口号
		 * 
		 * @param fileName 文件名
		 * 
		 * @param startPos 传送开始位置
		 * 
		 * @param Length 传送长度
		 * 
		 * @param wrtFile 写入的文件文件名
		 */

		public Builder setHostname(String hostname)
		{
			this.hostname = hostname;
			return this;
		}

		public Builder setPort(int port)
		{
			this.port = port;
			return this;
		}

		public Builder setRequiredFileName(String requiredFileName)
		{
			this.requiredFileName = requiredFileName;
			return this;
		}

		public Builder setStartPos(long startPos)
		{
			this.startPos = startPos;
			this.startPosStr = String.valueOf(this.startPos);
			return this;
		}

		public Builder setTransferLength(long transferLength)
		{
			this.transferLength = transferLength;
			this.transferLengthStr = String.valueOf(this.transferLength);
			return this;
		}

		public Builder setWriteToFileName(String writeToFileName)
		{
			this.writeToFileName = writeToFileName;
			return this;
		}

		public Builder setTransferLengthInfo(TransferLengthInfo transferLengthInfo)
		{
			this.transferLengthInfo = transferLengthInfo;
			return this;
		}

		/**
		 * 建造一个TransferThreadClient
		 * 
		 * @return
		 */
		public TransferThreadClient build()
		{
			if (this.transferLengthInfo == null)
			{
				this.transferLengthInfo = new TransferLengthInfo();
			}

			return new TransferThreadClient(this);
		}

	}

	// ******* 建造者 end ×××××××××

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

		} catch (UnknownHostException e)
		{
			// TODO Auto-generated catch block
			e.printStackTrace();
		} catch (IOException e)
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
				transferLengthInfo.addValue(len);

				totalLen += len;
				raf.write(buffer, 0, len);
				len = inFromServer.read(buffer);
			}

			logger.info("[TransferThreadClient] FilePointer!" + raf.getFilePointer());
			logger.info("[TransferThreadClient] Transfer Over!");
			logger.info("[TransferThreadClient] Total length:" + totalLen);
		} catch (IOException e)
		{
			// TODO Auto-generated catch block
			e.printStackTrace();
		} finally
		{
			// 关闭随机读写流
			try
			{
				if (raf != null)
				{
					raf.close();
				}
			} catch (IOException e)
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
		} catch (IOException e)
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
	//private long transferLength = 0;
	// 字符串型的传送长度
	private String transferLengthStr = null;
	// 请求传送的文件的文件名
	private String requiredFileName = null;
	// 写如的文件名
	private String writeToFileName = null;
	// 文件随机读写流
	private RandomAccessFile raf = null;

	private MyLogger logger = null;

	private TransferLengthInfo transferLengthInfo = null;
}
