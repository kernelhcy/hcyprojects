package fibonacci;

import java.math.BigInteger;

public class Main
{

	/**
	 * @param args
	 */
	public static void main(String[] args) {
		// TODO Auto-generated method stub
		Main m = new Main();
//		m.test(10, Fibonacci.RECURRENCE);
//		m.test(10, Fibonacci.ORDER);
		//m.test(100, Fibonacci.RECURRENCE);
//		m.test(100000, Fibonacci.ORDER);
		m.test(1000000, Fibonacci.DIVIDECONQUER);
//		m.test(10000, Fibonacci.ORDER);
//		m.test(100000, Fibonacci.ORDER);
	}
	
	private void test(int n, int method)
	{
		long start = System.currentTimeMillis();
		long end;
		BigInteger re = fb.getFN(n, method);
		end = System.currentTimeMillis();
		System.out.println("f(" + n + ")=" + re);
		System.out.println("Cost time: " + ((double)(end - start) / 1000) + "s");
	}
	private Fibonacci fb = new Fibonacci();

}
