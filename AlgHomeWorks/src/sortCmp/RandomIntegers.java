package sortCmp;

import java.util.Random;

/**
 * 产生0~2^32-1范围内的随机数。
 * @author hcy
 *
 */
public class RandomIntegers
{
	public RandomIntegers()
	{
		random = new Random();
	}
	
	/**
	 * 获得n个32位的随机数。
	 * @param n
	 * @return
	 */
	public void getIntegers(int[] array)
	{		
		if(array == null){
			return;
		}
		int n = array.length;
//		System.out.println("Need memory:" + (n / 1024 / 1024 *4)
//				+ "M.");

		for(int i = 0; i < n; ++i){
			array[i] = getNextUint();
		}
	}
	
	/**
	 * 获得一个0~ 2^32-1范围内的随机数。
	 * 使用两个随机的int数的低16位拼接成一个32位的随机数。
	 * @return
	 */
	private int getNextUint()
	{
		int re = 0;
		int leftPart, rightPart;
		leftPart = random.nextInt();
		rightPart = random.nextInt();
		re = leftPart;
		re <<= 16;
		rightPart &= 0xffff;	//取rightPart的低16位
		re += rightPart;
		return re;
	}
	
	private Random random;
}
