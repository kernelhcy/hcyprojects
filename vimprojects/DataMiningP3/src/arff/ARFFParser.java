/**
 * arff文件解析器。
 *
 */
package arff;

import java.io.*;

/**
 * 用于解析Arff文件。
 * 不是线程安全的！
 */
public class ARFFParser
{
	private ARFFParser()
	{
	}
	
	/**
	 * 返回f对应的arff文件对应的Arff对象。
	 */
	public static Arff parse(File f)
	{
		if(f == null){
			return null;
		}
		
		BufferedReader br = null;
		FileReader fr = null;
		
		arff = new Arff();
		String tmp;
		stage = 1;
		
		try{
			fr = new FileReader(f);
			br = new BufferedReader(fr);
			tmp = br.readLine();
			while(tmp != null){
				//System.out.printf("%s\n", tmp);
				parserLine(tmp);
				tmp = br.readLine();
			}
			
		}catch(IOException e){
			System.out.printf("IO ERROR!! In ARFFParser.");
			return null;
		}finally{
			try{
				fr.close();
				br.close();
			}catch(IOException e1){}
		}
		
		return arff;
	}
	
	/**
	 * 根据文件名fileName，解析一个arff文件
	 * 返回文件对应的Arff对象。
	 */
	public static Arff parse(String fileName)
	{
		if(fileName == null || !fileName.endsWith(".arff")){
			System.err.printf("File %s is not a arff file.\n"
					, fileName);
			return null;
		}
		
		File f = new File(fileName);
		if(!f.exists() || !f.isFile()){
			System.err.printf("File %s NOT exist or NOT a file.\n"
					, fileName);
			return null;
		}
		
		return parse(f);
	}
	
	/**
	 * 解析文件中的一行。出错就返回false。
	 */
	private static boolean parserLine(String line)
	{
		if(line == null){
			return true;
		}
		//第一行的没用。
		if(line.startsWith("@relation")){
			arff.relation = line.substring(10, line.length() - 1);
			return true;
		}
		//此行包含属性信息。
		if(line.startsWith("@attribute")){
			stage = 3;
			parserAttribute(line);
		}
		
		//此行是数据
		if(line.startsWith("@data")){
			stage = 4;
			return true;
		}
		if(stage == 4){
			parserDate(line);
		}
		return true;
	}
	
	/**
	 * 解析属性行
	 */
	private static boolean parserAttribute(String line)
	{
		//@attribute sex {Female,Male}
		//@attribute capital-gain numeric
		if(line == null){
			return true;
		}
		
		int index = 0; //字符串的当前位置。
		//跳过"@attribute"
		index += "@attribute".length();
		
		//跳过空白。
		while(Character.isWhitespace(line.charAt(index))) ++index;
		
		//遇到了属性的名字
		String name;
		int nameStart = index; //名字开始的位置。
		while(!Character.isWhitespace(line.charAt(index))) ++index;
		//此时index直向属性名字后面的空格。
		name = line.substring(nameStart, index);
		
		//下面解析属性值。
		//跳过空白。
		while(Character.isWhitespace(line.charAt(index))) ++index;
		if(line.charAt(index) == '{'){	//是categorical类型属性
			//@attribute sex {Female,Male}
			AttributeClass ac = new AttributeClass(
						AttributeType.CATE, name);
						
			String cateName;
			int cateStart;
			while(line.charAt(index) != '}'){
				++index;
				cateStart = index;
				while(line.charAt(index) != ','
					&& line.charAt(index) != '}'){
					++index;
				}
				cateName = line.substring(cateStart, index);
				ac.addCate(cateName);
			}
			arff.addAttrClass(ac);	
		}else{ //是numeric类型属性
			arff.addAttrClass(
				new AttributeClass(AttributeType.NUME, name));
		}
		
		return true;
	}
	
	/**
	 * 解析数据行
	 */
	private static boolean parserDate(String line)
	{
		if(line == null){
			return true;
		}
		Item item = new Item();
		Attribute attr = null;
		int index = 0;
		int si;
		String data;
		int nData;
		/*
		 * 根据解析出来的属性的个数和类别，解析数据。
		 */
		for(AttributeClass ac : arff.aclasses){
			si = index;
			while(index < line.length()
				&& line.charAt(index) != ',') 
				++index;	
			
			data = line.substring(si, index);

			if(ac.type == AttributeType.NUME){
				nData = Integer.valueOf(data);
				attr = new Attribute(ac, nData);
			}else{
				attr = new Attribute(ac, ac.getCateIndex(data));
			}
			
			item.addAttr(attr);
			++index;
		}
		
		arff.addItem(item);
		return true;
	}
	
	/*
	 * 记录现在所解析的文件的内容。
	 * 1表示开始解析， 2表示解析关系，3表示解析属性，4表示解析数据
	 */
	private static int stage; 
	
	//解析出来的Arff实例。
	private static Arff arff;
}
