package edu.xjtu.se.hcy.test;

/**
 * 
 * B继承自MyIncrement，同时需要实现接口Increment。
 * 在MyIncrement和Increment都有函数increment()。MyIncrement中的increment()是自己的函数，用来处理自己的成员变量
 * 和B没有关系，但是在B中实现increment()，仅仅是覆盖了MyIncrement中的increment()，没有实现Increment的increment().
 * 
 * 此问题可用内部类解决。
 * 
 * @author hcy
 *
 */
public class B extends MyIncrement
{
	private int x=0;
	
	static
	{
		System.out.println("static block is running...");
	}
	
	public void print()
	{
		System.out.println("Using Class B");
	}
	
	private void inc()
	{
		++x;
		System.out.println("B.x:"+x);
	}
	
	/**
	 * 通过Increment接口获得Closure类的实例。
	 * 可以通过调用Closure的increment方法来模拟B实现Increment的increment方法。
	 * 
	 * @return
	 */
	public Increment getInstanceClosure()
	{
		return new Closure();
	}
//	/**
//	 * 覆盖了MyIncrement的increment()方法，但没有实现Increment的方法。
//	 */
//	public void increment()
//	{
//		inc();
//		
//	}
	
	/**
	 * 内部类
	 * 私有
	 * 实现了Increment接口，并实现其中的increment()方法。
	 * 
	 */
	private class Closure implements Increment
	{

		@Override
		public void increment()
		{
			//仅仅是调用外部类B的一个方法inc()。
			inc();
			
		}
		
		
	}
	
}
