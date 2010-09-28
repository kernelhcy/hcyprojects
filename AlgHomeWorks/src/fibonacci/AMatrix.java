package fibonacci;

import java.math.BigInteger;

public class AMatrix
{
	public AMatrix(String s00, String s01, String s10, String s11)
	{
		matrix = new BigInteger[2][2];
		matrix[0][0] = new BigInteger(s00);
		matrix[0][1] = new BigInteger(s01);
		matrix[1][0] = new BigInteger(s10);
		matrix[1][1] = new BigInteger(s11);
	}
	
	public AMatrix()
	{
		this("1", "1", "1", "0");
	}
	
	public AMatrix(AMatrix m)
	{
		matrix = new BigInteger[2][2];
		this.copy(m);
	}
	
	private void copy(AMatrix m)
	{
		matrix[0][0] = new BigInteger(m.matrix[0][0].toByteArray());
		matrix[0][1] = new BigInteger(m.matrix[0][1].toByteArray());
		matrix[1][0] = new BigInteger(m.matrix[1][0].toByteArray());
		matrix[1][1] = new BigInteger(m.matrix[1][1].toByteArray());
	}
	
	/**
	 * 求本矩阵的n次方
	 * @param m
	 * @param n
	 * @return
	 */
	public AMatrix matrixPow(int n)
	{
		AMatrix tmp = new AMatrix(this);
		AMatrix re = new AMatrix(this);
		for(; n > 0; n >>=1){
			if (n % 2 == 1){
				re.multiply(tmp);
			}
			tmp.multiply(tmp);
		}
		return re;
	}
	
	private AMatrix multiply(AMatrix m)
	{
		AMatrix tmpM = new AMatrix(); //防止自己乘自己
		for(int i = 0; i < 2; ++i){
			for(int j = 0; j < 2; ++j){
				BigInteger tmp = new BigInteger("0");
				for(int k = 0; k < 2; ++k){
					tmp.add(matrix[i][k]
					   .multiply(m.matrix[k][j]));
				}
				tmpM.matrix[i][j]= tmp;
			}
		}
		this.copy(tmpM);
		return this;
	}
	
	private BigInteger[][] matrix;
	private AMatrix Identity = new AMatrix(); //单位矩阵
}
