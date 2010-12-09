/**
 * arff�ļ���������
 *
 */
package arff;

import java.io.*;

/**
 * ���ڽ���Arff�ļ���
 * �����̰߳�ȫ�ģ�
 */
public class ARFFParser
{
	private ARFFParser()
	{
	}
	
	/**
	 * ����f��Ӧ��arff�ļ���Ӧ��Arff����
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
	 * �����ļ���fileName������һ��arff�ļ�
	 * �����ļ���Ӧ��Arff����
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
	 * �����ļ��е�һ�С�����ͷ���false��
	 */
	private static boolean parserLine(String line)
	{
		if(line == null){
			return true;
		}
		//��һ�е�û�á�
		if(line.startsWith("@relation")){
			arff.relation = line.substring(10, line.length() - 1);
			return true;
		}
		//���а���������Ϣ��
		if(line.startsWith("@attribute")){
			stage = 3;
			parserAttribute(line);
		}
		
		//����������
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
	 * ����������
	 */
	private static boolean parserAttribute(String line)
	{
		//@attribute sex {Female,Male}
		//@attribute capital-gain numeric
		if(line == null){
			return true;
		}
		
		int index = 0; //�ַ����ĵ�ǰλ�á�
		//����"@attribute"
		index += "@attribute".length();
		
		//�����հס�
		while(Character.isWhitespace(line.charAt(index))) ++index;
		
		//���������Ե�����
		String name;
		int nameStart = index; //���ֿ�ʼ��λ�á�
		while(!Character.isWhitespace(line.charAt(index))) ++index;
		//��ʱindexֱ���������ֺ���Ŀո�
		name = line.substring(nameStart, index);
		
		//�����������ֵ��
		//�����հס�
		while(Character.isWhitespace(line.charAt(index))) ++index;
		if(line.charAt(index) == '{'){	//��categorical��������
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
		}else{ //��numeric��������
			arff.addAttrClass(
				new AttributeClass(AttributeType.NUME, name));
		}
		
		return true;
	}
	
	/**
	 * ����������
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
		 * ���ݽ������������Եĸ�������𣬽������ݡ�
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
	 * ��¼�������������ļ������ݡ�
	 * 1��ʾ��ʼ������ 2��ʾ������ϵ��3��ʾ�������ԣ�4��ʾ��������
	 */
	private static int stage; 
	
	//����������Arffʵ����
	private static Arff arff;
}
