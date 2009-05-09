package tcp.test;

import java.io.BufferedWriter;
import java.io.File;
import java.io.FileWriter;
import java.io.IOException;

public class CreateFile
{
	
	/**
	 * @param args
	 * @throws IOException 
	 */
	public static void main(String[] args) throws IOException
	{
		// TODO Auto-generated method stub
		File file = new File("test.jpg");
		
		file.delete();
		file.createNewFile();
		file.canWrite();
		
		BufferedWriter bw = new BufferedWriter(new FileWriter(file));
		System.out.println("writting...");
		int cnt = 0;
		
		while(cnt < 10000000)
		{
			++cnt;
			bw.write("datadatadatadatadatadatadatadatadatadatadatadatadatadatadatadatadatadatadatadatadata");
			
			//bw.flush();
		}
		bw.flush();
		bw.close();
		System.out.println("over");
	}
	
}
