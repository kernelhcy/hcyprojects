package edu.xjtu.se.hcy.test;

import java.util.ArrayList;
import java.util.List;

public class Main
{
	
	public static void main(String[] args) throws Exception
	{
		Class c=null;
		Package p=null;
		c=Class.forName("edu.xjtu.se.hcy.test.A");
		p=c.getPackage();
		System.out.println(p.getName());
		
		Object a_inst=c.newInstance();
		((A)a_inst).print();
		
		System.out.println("Test Inner Class.");
		
		A a=new A();
		B b=new B();
		
		a.increment();
		//调用父类MyIncrement的increment()方法。
		b.increment();
		//调用实现接口Increment的increment()方法
		b.getInstanceClosure().increment();
		
		List<String> list=new ArrayList<String>();
		
		list.add("a");
		list.add("b");
		list.add("c");
		list.add("d");
		list.add("e");
		list.add("f");
		list.add("g");
		
		for(String s:list)
		{
			System.out.println("list : "+s);
		}
	}
}