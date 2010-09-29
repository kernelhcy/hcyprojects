package fibonacci;

import java.math.BigInteger;

public class Fibonacci
{
	public static final int RECURRENCE 	= 1;
	public static final int ORDER 		= 2;
	public static final int DIVIDECONQUER 	= 3;
	
	public Fibonacci()
	{
		
	}
	
	public BigInteger getFN(int n, int method)
	{
		BigInteger re = null;
		switch (method) {
		case RECURRENCE:
			re = this.getFNRECU(n);
			break;
		case ORDER:
			re = this.getFNORDER(n);
			break;
		case DIVIDECONQUER:
			re = this.getFNDC(n);
			break;
		default:
			break;
		}
		return re;
	}
	
	/**
	 * 递归求解。
	 * @param n
	 * @return
	 */
	private BigInteger getFNRECU(int n)
	{
		if(n == 1){
			return new BigInteger("1");
		}
		if(n == 0){
			return new BigInteger("0");
		}
		
		return getFNRECU(n - 1).add(getFNRECU(n - 2));
	}
	
	/**
	 * 递推求解。
	 * @param n
	 * @return
	 */
	private BigInteger getFNORDER(int n)
	{
		BigInteger prepre = new BigInteger("0");
		BigInteger pre = new BigInteger("1");
		BigInteger re = null;
		
		if(n == 0){
			return prepre;
		}
		if(n == 1){
			return pre;
		}
		
		for(int i = 2; i <= n; ++i){
			re = pre.add(prepre);
			prepre = pre;
			pre = re;
		}
		
		return re;
	}
	
	private BigInteger getFNDC(int n)
	{
		BigInteger re = null;
		Matrix am = new Matrix(2);
		am.matrix[0][0] = new BigInteger("1");
		am.matrix[0][1] = new BigInteger("1");
		am.matrix[1][0] = new BigInteger("1");
		am.matrix[1][1] = new BigInteger("0");
		am = am.matrixPow(n - 1);
		re = am.matrix[0][0];
		return re;
	}
	
	private int n;
}
