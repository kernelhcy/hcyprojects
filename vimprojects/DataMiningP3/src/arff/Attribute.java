/**
 * ��ʾһ�����Լ����Ӧ��ֵ��
 */
package arff;

public class Attribute
{
	public AttributeClass aclass; 	//���Զ�Ӧ�����
	public int val;			//����ֵ������������ԣ���ʾ��Ӧ���ֵ�ı�š�
	
	public Attribute(AttributeClass c, int val)
	{
		this.aclass = c;
		this.val = val;
	}
	
	/*
	 * ������������֮�������ȡ�
	 */
	public double distance(Attribute a)
	{
		if(a == null || a.aclass != aclass){
			return -1.0;
		}
		if(aclass.type == AttributeType.CATE){
			if(a.val == val){
				return 1.0;
			}
		}
		if(aclass.type == AttributeType.NUME){
			return Math.abs(a.val - val);
		}
		return -1.0;
	}
		
	public String toString()
	{
		if(aclass.type == AttributeType.NUME){
			return ((Integer)val).toString();
		}else{
			return aclass.getCate(val);
		}
	}
}
