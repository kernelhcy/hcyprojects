package edu.xjtu.se.hcy.secondencrypt;

import java.math.BigInteger;
import java.util.Map;

/**
 * 流程分析： 
 * 1.甲方构建密钥对儿，将公钥公布给乙方，将私钥保留。
 * 2.甲方使用私钥加密数据，然后用私钥对加密后的数据签名，发送给乙方签名以及加密后的数据；
 * 3.乙方使用公钥、签名来验证待解密数据是否有效，如果有效使用公钥对数据解密。
 * 4.乙方使用公钥加密数据，向甲方发送经过加密后的数据；甲方获得加密数据，通过私钥解密。
 */
public class RSACoderTest
{
	private byte[] publicKey;
	private byte[] privateKey;
	private RSACoder rsa = null;
	private String inputStr = "黄丛宇06161032";
	
	public RSACoderTest(String input)
	{
		this.inputStr = input;
		try
		{
			setUp();
		}
		catch (Exception e) 
		{
			System.err.println("Create key error." + e.getMessage());
		}
	}
	
	/**
	 * 以16进制的形式打印数据
	 * @param data
	 * @return
	 */
	private void print(byte[] data)
	{
		if (null == data)
		{
			return;
		}
		System.out.println(new BigInteger(data).toString(16));
		return;
	}
	
	private void setUp() throws Exception
	{
		rsa = new RSACoder();
		Map<String, Object> keyMap = rsa.initKey();
		
		publicKey = rsa.getPublicKey(keyMap);
		privateKey = rsa.getPrivateKey(keyMap);
		System.out.print("公钥: "); print(publicKey);
		System.out.print("密钥: "); print(privateKey);
	}
	
	public void testEncrypt() throws Exception
	{
		System.out.println("\n公钥加密——私钥解密:");
		byte[] data = inputStr.getBytes();
		byte[] encodedData = rsa.encryptByPublicKey(data, publicKey);
		System.out.print("加密后: "); print(encodedData);
		
		byte[] decodedData = rsa.decryptByPrivateKey(encodedData, privateKey);
		String outputStr = new String(decodedData);
		System.out.println("解密后: " + outputStr);
		
		System.out.println("\n私钥加密——共钥解密:");
		encodedData = rsa.encryptByPrivateKey(data, privateKey);
		System.out.print("加密后: "); print(encodedData);
		
		decodedData = rsa.decryptByPublicKey(encodedData, publicKey);
		outputStr = new String(decodedData);
		System.out.println("解密后: " + outputStr);
	}
	
	public void testSign() throws Exception
	{
		System.out.println("\n私钥签名——公钥验证签名");
		byte[] encodedData = rsa.encryptByPrivateKey(inputStr.getBytes(), privateKey);
		// 产生签名
		byte[] sign = rsa.sign(encodedData, privateKey);
		System.out.print("签名:"); print(sign);
		
		// 验证签名
		boolean status = rsa.verify(encodedData, publicKey, sign);
		System.out.println("验证状态:" + status);
		
	}
	
}
