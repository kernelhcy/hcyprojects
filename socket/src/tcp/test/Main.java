package tcp.test;

import tcp.client.TCPClientMain;

public class Main
{
	
	/**
	 * @param args
	 * @throws Exception 
	 */
	public static void main(String[] args) throws Exception
	{
		int cnt=0;
		
		while (cnt<100)
		{
			++cnt;
			// TODO Auto-generated method stub
			TCPClientMain cm = new TCPClientMain("localhost", 1234);
			
			cm.connect();
			
			Thread.sleep(10);
		}
	}
	
}
