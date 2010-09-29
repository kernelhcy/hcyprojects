package fibonacci;

import java.math.BigInteger;

/**
 * 方阵
 * @author hcy
 *
 */
public class Matrix
{
	public Matrix(int n)
	{
		matrix = new BigInteger[n][n];
		this.n = n;
		for(int i = 0; i < n; ++i){
			for(int j = 0; j < n; ++j){
				matrix[i][j] = new BigInteger("0");
			}
		}
	}
	
	
	public Matrix(Matrix m)
	{
		this.copy(m);
	}
	
	private void copy(Matrix m)
	{
		matrix = new BigInteger[m.n][m.n];
		this.n = m.n;
		for(int i = 0; i < this.n; ++i){
			for(int j = 0; j < this.n; ++j){
				matrix[i][j] = 
				new BigInteger(m.matrix[i][j].toByteArray());
			}
		}
	}
	
	/**
	 * 获得单位矩阵
	 * @return
	 */
	public Matrix getIdentity()
	{
		if(Identity == null){
			Identity = new Matrix(n);
			for(int i = 0; i < n; ++i){
				Identity.matrix[i][i] = new BigInteger("1");
			}
		}
		return Identity;
	}
	
	/**
	 * 求本矩阵的n次方
	 * @param m
	 * @param n
	 * @return
	 */
	public Matrix matrixPow(int n)
	{
		
		Matrix tmp = null;
		if(n == 1){
			return new Matrix(this);
		}
//		AMatrix re = new AMatrix(this);
//		for(; n > 0; n >>=1){
//			if (n % 2 == 1){
//				re.multiply(tmp);
//			}
//			tmp.multiply(tmp);
//		}
		if(n % 2 == 1){
			tmp = matrixPow(n - 1);
			return this.multiply(tmp);
		}
		else{
			tmp = matrixPow(n / 2);
			return tmp.multiply(tmp);
		}
	}
	
	public void show(){
		for(int i = 0; i < n; ++i){
			for(int j = 0; j < n; ++j){
				System.out.print(matrix[i][j].toString() 
						+ " ");
			}
			System.out.println();
		}
	}
	
	private Matrix multiply(Matrix m)
	{
		Matrix tmpM = new Matrix(m.n); //防止自己乘自己
		for(int i = 0; i < n; ++i){
			for(int j = 0; j < n; ++j){
				BigInteger tmp = new BigInteger("0");
				for(int k = 0; k < n; ++k){
					tmp = tmp.add(matrix[i][k]
					   .multiply(m.matrix[k][j]));
				}
				tmpM.matrix[i][j]= tmp;
			}
		}
		//this.copy(tmpM);
		return tmpM;
	}
	
	public BigInteger[][] matrix;
	public int n;
	private static Matrix Identity = null;
}
