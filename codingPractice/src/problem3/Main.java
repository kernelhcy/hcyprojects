package problem3;

import java.io.BufferedReader;
import java.io.BufferedWriter;
import java.io.File;
import java.io.FileWriter;
import java.io.IOException;
import java.io.InputStreamReader;
import java.util.Arrays;

public class Main
{

	/**
	 * @param args
	 */
	public static void main(String[] args)
	{
		// TODO Auto-generated method stub
		Main m = new Main();
		m.run();
	}

	public Main() {

	}

	public void run()
	{
		System.out.println("Input the string:(split with '#' and end with ';')");
		readData();
		System.out.println("Sort...");
		Arrays.sort(ss);
		System.out.println("Output to file(a.out)...");
		File f = new File("a.out");
		try {
			if (!f.exists()) {
				f.createNewFile();
			}
			BufferedWriter bw = new BufferedWriter(
					new FileWriter(f));
			for (String s : ss) {
				bw.write(s);
				bw.newLine();
			}
			bw.flush();
			bw.close();
		}
		catch (Exception e) {
			// TODO: handle exception
		}
		System.out.println("The number of strings : " + ss.length);
		int longestString, shortestString;
		longestString = shortestString = 0;
		for(int i = 1; i < ss.length; ++i){
			if(ss[i].length() > ss[longestString].length()){
				longestString = i;
			}
			
			if(ss[i].length() < ss[shortestString].length()){
				shortestString = i;
			}
		}
		
		System.out.println("The longest string is : " 
				+ ss[longestString]);
		System.out.println("The shortest string is : " 
				+ ss[shortestString]);
	}

	private int readData()
	{
		int len = 0;
		BufferedReader br = new BufferedReader(
				new InputStreamReader(System.in));

		try {
			ins = br.readLine();
		}
		catch (IOException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
		String tmp;
		tmp = ins.substring(0, ins.indexOf(';'));
		// 分割字符串
		ss = tmp.split("#");
		for (String s : ss) {
			System.out.print(s);
			System.out.print(" ");
		}
		System.out.println();
		return len;
	}

	private String ins;
	private String[] ss;
}
