package edu.xjtu.se.hcy.secondencrypt;

import java.math.BigInteger;

public class Main
{
	
	/**
	 * 以16进制的形式打印数据
	 * @param data
	 * @return
	 */
	private static void print(byte[] data)
	{
		if (null == data)
		{
			return;
		}
		System.out.println(new BigInteger(data).toString(16));
		return;
	}
	/**
	 * @param args
	 * @throws Exception 
	 * 加密，签名，认证
	 */
	public static void main(String[] args) throws Exception
	{
		
		String inputStr = "黄丛宇06161032\n";
		String outputStr = null;
		System.out.println("原文:\n" + inputStr);
		byte[] input = inputStr.getBytes();
		byte[] output = null;

		
		Coder coder = new Coder();
		output = coder.encryptMD5(input);
		System.out.println("MD5:");
		print(output);
		
		output = coder.encryptSHA(input);
		System.out.println("SHA:");
		print(output);
		
		byte[] key = coder.initMacKey();
		output = coder.encryptHMAC(input, key);
		System.out.println("HMAC:");
		print(output);
		
		//test DES
		key = null;
		DESCoder descoder = null;
		// ECB
		System.out.println("\nTest DES.");
		descoder = new DESCoder();
        key = descoder.initKey();   
        System.out.println("密钥:"); 
        print(key);
        
        output = descoder.encrypt(input, key);  
        System.out.print("加密后:");
        print(output);
        output = descoder.decrypt(output, key);  
        outputStr = new String(output);  
        System.out.println("解密后: " + outputStr);  
       
        System.out.println("Test RSA.");
        //加密数据的长度不能超过117bytes
        RSACoderTest rsatester = new RSACoderTest("06161032 黄丛宇 软件62\n");
        rsatester.testEncrypt();
        rsatester.testSign();
	}
	
}
