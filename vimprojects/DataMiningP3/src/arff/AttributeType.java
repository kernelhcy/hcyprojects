/**
 * 属性的类型。
 */
 
package arff;

public enum AttributeType
{
	CATE{	//categorical
		public String getName()
		{
			return "categorical";
		}
	}, 
	NUME{	//numeric
		public String getName()
		{
			return "numeric";
		}
	};
	
	public abstract String getName();
}
