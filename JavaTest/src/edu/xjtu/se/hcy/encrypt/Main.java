package edu.xjtu.se.hcy.encrypt;

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
		
		String inputStr = "黄丛宇06161032\n" +
				"1.加解密与认证（必选）\n" +
				"要求：结合所提供密码学开发库或利用Java，vc自带密码学开发库完成如下要求\n" +
				"1）设计实现消息的机密性，完整性与可鉴别要求。\n" +
				"2）报告中要有思路分析，设计流程与关键代码。每步变换后的结果\n" +
				"3）要求结合个人信息进行，如消息中应包含个人学号，姓名等\n" +
				"4）应使用到消息分组，不能太短\n";
		String outputStr = null;
		System.out.println("原文:\n" + inputStr);
		byte[] input = inputStr.getBytes();
		System.out.println("长度:" + input.length * 8 + "bit");
		byte[] output = null;
//		String code = Coder.encryptBASE64(inputData);
//		System.out.println("BASE64加密后:\t" + code);
//		
//		byte[] output = Coder.decryptBASE64(code);
//		String outputStr = new String(output);
//		System.out.println("BASE64解密后:\t" + outputStr);
		
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
		System.out.println("\nTest DES ECB.");
		descoder = new DESCoder(DESCoder.MODE_ECB);
        key = descoder.initKey();   
        System.out.println("密钥:"); 
        print(key);
        
        output = descoder.encrypt(input, key);  
        System.out.print("加密后:");
        print(output);
        output = descoder.decrypt(output, key);  
        outputStr = new String(output);  
        System.out.println("解密后: " + outputStr);  
        
        //CBC
        System.out.println("Test DES CBC.");
		descoder = new DESCoder(DESCoder.MODE_CBC);
        key = descoder.initKey();   
        System.out.println("密钥:"); 
        print(key);
        
        output = descoder.encrypt(input, key);  
        System.out.print("加密后:");
        print(output);
        output = descoder.decrypt(output, key);  
        outputStr = new String(output);  
        System.out.println("解密后: " + outputStr);  
        
        //CFB
        System.out.println("Test DES CFB.");
		descoder = new DESCoder(DESCoder.MODE_CFB);
        key = descoder.initKey();   
        System.out.println("密钥:"); 
        print(key);
        
        output = descoder.encrypt(input, key);  
        System.out.print("加密后:");
        print(output);
        output = descoder.decrypt(output, key);  
        outputStr = new String(output);  
        System.out.println("解密后: " + outputStr);  
        
        //OFB
        System.out.println("Test DES OFB.");
		descoder = new DESCoder(DESCoder.MODE_OFB);
        key = descoder.initKey();   
        System.out.println("密钥:"); 
        print(key);
        
        output = descoder.encrypt(input, key);  
        System.out.print("加密后:");
        print(output);
        output = descoder.decrypt(output, key);  
        outputStr = new String(output);  
        System.out.println("解密后: " + outputStr);  
        
        //CTR
		System.out.println("\nTest DES CTR.");
		descoder = new DESCoder(DESCoder.MODE_CTR);
        key = descoder.initKey();   
        System.out.println("密钥:"); 
        print(key);
        
        output = descoder.encrypt(input, key);  
        System.out.print("加密后:");
        print(output);
        output = descoder.decrypt(output, key);  
        outputStr = new String(output);  
        System.out.println("解密后: " + outputStr);  
        //test PBE
//        System.out.println("Test PBE"); 
//        System.out.println("原文: " + inputStr);  
//        byte[] input = inputStr.getBytes();  
//  
//        String pwd = "abc123";  
//        System.out.println("密码: " + pwd);  
//  
//        byte[] salt = PBECoder.initSalt();  
//        byte[] data = PBECoder.encrypt(input, pwd, salt);  
//        System.out.println("加密后: " + PBECoder.encryptBASE64(data));  
//  
//        output = PBECoder.decrypt(data, pwd, salt);  
//        outputStr = new String(output);  
//        System.out.println("解密后: " + outputStr);   
        
        System.out.println("Test RSA.");
        //加密数据的长度不能超过117bytes
        RSACoderTest rsatester = new RSACoderTest("06161032 黄丛宇 软件62\n");
        rsatester.testEncrypt();
        rsatester.testSign();
        
//        System.out.println("Test DH.");
//        DHCoderTest dhtester = new DHCoderTest();
//        dhtester.test();
//        
//        System.out.println("Test DSA");
//        DSACoderTest dsatester = new DSACoderTest();
//        dsatester.test();
//        
//        System.out.println("Test ECC.");
//        ECCCoderTest ecctester = new ECCCoderTest();
//        ecctester.test();
//        
//        System.out.println("Test Certificate.");
//        CertificateCoderTest cctester = new CertificateCoderTest();
//        cctester.test();
//        cctester.testSign();
        
	}
	
}
