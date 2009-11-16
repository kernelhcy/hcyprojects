package edu.xjtu.se.hcy.encrypt;

import java.math.BigInteger;

public class Main
{
	
	/**
	 * @param args
	 * @throws Exception 
	 */
	public static void main(String[] args) throws Exception
	{
		
		String inputStr = "简单加密";
		System.err.println("原文:\n" + inputStr);
		
		byte[] inputData = inputStr.getBytes();
		String code = Coder.encryptBASE64(inputData);
		
		System.err.println("BASE64加密后:\n" + code);
		
		byte[] output = Coder.decryptBASE64(code);
		
		String outputStr = new String(output);
		
		System.err.println("BASE64解密后:\n" + outputStr);
		
		String key = Coder.initMacKey();
		System.err.println("Mac密钥:\n" + key);
		
		BigInteger md5 = new BigInteger(Coder.encryptMD5(inputData));
		System.err.println("MD5:\n" + md5.toString(16));
		
		BigInteger sha = new BigInteger(Coder.encryptSHA(inputData));
		System.err.println("SHA:\n" + sha.toString(32));
		
		BigInteger mac = new BigInteger(Coder.encryptHMAC(inputData, inputStr));
		System.err.println("HMAC:\n" + mac.toString(16));
		
		//test DES
		System.out.println("Test EDS.");
		inputStr = "DES";  
        key = DESCoder.initKey();  
        System.err.println("原文:\t" + inputStr);  
  
        System.err.println("密钥:\t" + key);  
  
        inputData = inputStr.getBytes();  
        inputData = DESCoder.encrypt(inputData, key);  
  
        System.err.println("加密后:\t" + DESCoder.encryptBASE64(inputData));  
  
        byte[] outputData = DESCoder.decrypt(inputData, key);  
        outputStr = new String(outputData);  
  
        System.err.println("解密后:\t" + outputStr);  
        
        //test PBE
        System.out.println("Test PBE");
        inputStr = "abc";  
        System.err.println("原文: " + inputStr);  
        byte[] input = inputStr.getBytes();  
  
        String pwd = "efg";  
        System.err.println("密码: " + pwd);  
  
        byte[] salt = PBECoder.initSalt();  
  
        byte[] data = PBECoder.encrypt(input, pwd, salt);  
  
        System.err.println("加密后: " + PBECoder.encryptBASE64(data));  
  
        output = PBECoder.decrypt(data, pwd, salt);  
        outputStr = new String(output);  
  
        System.err.println("解密后: " + outputStr);   
	}
	
}
