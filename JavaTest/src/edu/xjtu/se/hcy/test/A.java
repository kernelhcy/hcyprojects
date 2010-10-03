package edu.xjtu.se.hcy.test;

public class A implements Increment
{
	private int x=0;
	
	static
	{
		System.out.println("static block in A...");
	}
	
	public void print()
	{
		System.out.println("Using Class A");
	}
	
	@Override
	public void increment()
	{
		// TODO Auto-generated method stub
		++x;
		System.out.println("A.x:"+x);
	}
}
