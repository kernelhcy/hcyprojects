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
		
		String inputStr = "黄丛宇06161032";
		System.out.println("原文:\t" + inputStr);
		
		byte[] inputData = inputStr.getBytes();
		String code = Coder.encryptBASE64(inputData);
		System.out.println("BASE64加密后:\t" + code);
		
		byte[] output = Coder.decryptBASE64(code);
		
		String outputStr = new String(output);
		System.out.println("BASE64解密后:\t" + outputStr);
		
		BigInteger md5 = new BigInteger(Coder.encryptMD5(inputData));
		System.out.println("MD5:\t" + md5.toString(16));
		
		BigInteger sha = new BigInteger(Coder.encryptSHA(inputData));
		System.out.println("SHA:\t" + sha.toString(32));
		
		BigInteger mac = new BigInteger(Coder.encryptHMAC(inputData, inputStr));
		System.out.println("HMAC:\t" + mac.toString(16));
		
		//test DES
		System.out.println("Test DES.");
		String key;
		key = Coder.initMacKey();
//		System.out.println("Mac密钥:\n" + key);
        key = DESCoder.initKey();  
        System.out.println("原文:\t" + inputStr);  
        System.out.println("密钥:\t" + key);  
  
        inputData = inputStr.getBytes();  
        inputData = DESCoder.encrypt(inputData, key);  
  
        System.out.println("加密后:\t" + DESCoder.encryptBASE64(inputData));  
  
        byte[] outputData = DESCoder.decrypt(inputData, key);  
        outputStr = new String(outputData);  
  
        System.out.println("解密后:\t" + outputStr);  
        
        //test PBE
        System.out.println("Test PBE"); 
        System.out.println("原文: " + inputStr);  
        byte[] input = inputStr.getBytes();  
  
        String pwd = "abc123";  
        System.out.println("密码: " + pwd);  
  
        byte[] salt = PBECoder.initSalt();  
        byte[] data = PBECoder.encrypt(input, pwd, salt);  
        System.out.println("加密后: " + PBECoder.encryptBASE64(data));  
  
        output = PBECoder.decrypt(data, pwd, salt);  
        outputStr = new String(output);  
        System.out.println("解密后: " + outputStr);   
        
        System.out.println("Test RSA.");
        RSACoderTest rsatester = new RSACoderTest();
        rsatester.test();
        rsatester.testSign();
        
        System.out.println("Test DH.");
        DHCoderTest dhtester = new DHCoderTest();
        dhtester.test();
        
        System.out.println("Test DSA");
        DSACoderTest dsatester = new DSACoderTest();
        dsatester.test();
        
        System.out.println("Test ECC.");
        ECCCoderTest ecctester = new ECCCoderTest();
        ecctester.test();
        
        System.out.println("Test Certificate.");
        CertificateCoderTest cctester = new CertificateCoderTest();
        cctester.test();
        cctester.testSign();
        
	}
	
}
