/**
 * ��ʾ������� �����������ͣ����Ե���ָ��Χ��
 */
package arff;

import java.util.ArrayList;

public class AttributeClass
{
	public AttributeType type;
	public String name; 			//���Ե�����
	
	public AttributeClass(AttributeType type, String name)
	{
		cates = new ArrayList<String>();
		this.type = type;
		this.name = name;
	}
	
	/**
	 * �������cate
	 * @parm cate : ��ʾ�����ַ���
	 */
	public void addCate(String cate)
	{
		cates.add(cate);	
	}
	
	/**
	 * ��ȡ���ֵ��Ӧ�ı�š�
	 */
	public int getCateIndex(String cate)
	{
		for(int i = 0; i < cates.size(); ++i){
			if(cate.compareTo(cates.get(i)) == 0){
				return i;
			}
		}
		return -1;
	}
	
	/*
	 * ���ط���ֵ��Ӧ�����ơ�
	 */
	public String getCate(int index)
	{
		if(index == -1){
			return "?"; //ֵȱʧ
		}
		return cates.get(index);
	}
	
	/**
	 * �ж��������Ƿ������������
	 */
	public boolean hasCate(String cate)
	{
		//��ֵ�������Ƿ���true
		if(type == AttributeType.NUME){
			return true;
		}
		return cates.contains(cate);
	}
	
	/*
	 * ���ط�������������ֵ�ĸ�����
	 */
	public int getCateNum()
	{
		return cates.size();
	}
	
	@Override
	public String toString()
	{
		StringBuilder sb = new StringBuilder("@Attribute ");
		sb.append(name);
		if(type == AttributeType.NUME){
			sb.append(" numeric\n");
			return sb.toString();
		}
		
		sb.append(" {");
		for(String s : cates){
			sb.append(s);
			sb.append(',');
		}
		sb.append("}\n");
		return sb.toString();
	}
	
	private ArrayList<String> cates; 	//categorical���Ͷ�Ӧ�����
	
}
