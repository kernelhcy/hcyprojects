package edu.xjtu.se.hcy.encrypt;

import java.util.Map;

/**
 * 
 * @author 梁栋
 * @version 1.0
 * @since 1.0
 * 
 * 流程分析： 
 * 1.甲方构建密钥对儿，将公钥公布给乙方，将私钥保留。
 * 2.甲方使用私钥加密数据，然后用私钥对加密后的数据签名，发送给乙方签名以及加密后的数据；
 * 3.乙方使用公钥、签名来验证待解密数据是否有效，如果有效使用公钥对数据解密。
 * 4.乙方使用公钥加密数据，向甲方发送经过加密后的数据；甲方获得加密数据，通过私钥解密。
 */
public class RSACoderTest
{
	private String publicKey;
	private String privateKey;
	
	public RSACoderTest()
	{
		try
		{
			setUp();
		}
		catch (Exception e) 
		{
			System.err.println("Create key error." + e.getMessage());
		}
	}
	
	private void setUp() throws Exception
	{
		Map<String, Object> keyMap = RSACoder.initKey();
		
		publicKey = RSACoder.getPublicKey(keyMap);
		privateKey = RSACoder.getPrivateKey(keyMap);
		System.out.println("公钥: \n\r" + publicKey);
		System.out.println("私钥： \n\r" + privateKey);
	}
	
	public void test() throws Exception
	{
		System.out.println("公钥加密——私钥解密");
		String inputStr = "黄丛宇06161032";
		byte[] data = inputStr.getBytes();
		byte[] encodedData = RSACoder.encryptByPublicKey(data, publicKey);
		byte[] decodedData = RSACoder.decryptByPrivateKey(encodedData, privateKey);
		
		String outputStr = new String(decodedData);
		System.out.println("加密前: " + inputStr + "\n\r" + "解密后: " + outputStr);
		
	}
	
	public void testSign() throws Exception
	{
		System.out.println("私钥加密——公钥解密");
		String inputStr = "黄丛宇06161032";
		byte[] data = inputStr.getBytes();
		byte[] encodedData = RSACoder.encryptByPrivateKey(data, privateKey);
		byte[] decodedData = RSACoder.decryptByPublicKey(encodedData, publicKey);
		
		String outputStr = new String(decodedData);
		System.out.println("加密前: " + inputStr + "\n\r" + "解密后: " + outputStr);
		
		System.out.println("私钥签名——公钥验证签名");
		// 产生签名
		String sign = RSACoder.sign(encodedData, privateKey);
		System.out.println("签名:\r" + sign);
		
		// 验证签名
		boolean status = RSACoder.verify(encodedData, publicKey, sign);
		System.out.println("状态:\r" + status);
		
	}
	
}
