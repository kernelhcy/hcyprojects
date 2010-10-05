package problem2;

import java.util.Arrays;

/**
 * 简单的栈
 * @author hcy
 *
 * @param <T>
 */
public class MyStack<T>
{
	public MyStack(int len)
	{
		array = new Object[len];
		size = 0;
	}
	
	public MyStack()
	{
		this(50);
	}
	
	/**
	 * 栈空返回null
	 * @return
	 */
	public T pop()
	{
		if(size == 0){
			return null;
		}
		--size;
		T re = (T)array[size];
		array[size] = null;
		
		return re;
	}
	
	/**
	 * 栈满直接返回。
	 * @param element
	 */
	public void push(T element)
	{
		if(size == array.length){
			System.out.println("Sorry... The stack is full!");
			//resize(array.length + 50);
			return;
		}
		System.out.println("push:" + element);
		array[size] = element;
		++size;
	}
	
	public int size()
	{
		return size;
	}
	
	public boolean full()
	{
		return size == array.length;
	}
	
	public boolean empty()
	{
		return size == 0;
	}
	
	public void print()
	{
		System.out.print("Stack:");
		for(int i = 0; i < size; ++i){
			System.out.print(array[i] + " ");
		}
		System.out.println();
	}
	
	/**
	 * 扩展栈
	 * @param len
	 * @return
	 */
	private int resize(int len)
	{
		if(len < array.length){
			return array.length;
		}
		array = Arrays.copyOf(array, len);
		return len;
	}
	
	private Object[] array;
	private int size;
}
