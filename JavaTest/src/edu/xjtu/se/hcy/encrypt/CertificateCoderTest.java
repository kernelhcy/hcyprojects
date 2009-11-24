package edu.xjtu.se.hcy.encrypt;

import java.math.BigInteger;

/**
 * 
 * @author 梁栋
 * @version 1.0
 * @since 1.0
 * 
 * 1.生成keyStroe文件 
 * 在命令行下执行以下命令： 
 * 		keytool -genkey -validity 36000 -alias www.hcy.org -keyalg RSA -keystore hcy.keystore
 * 其中 
 * -genkey表示生成密钥 
 * -validity指定证书有效期，这里是36000天 
 * -alias指定别名，这里是www.zlex.org 
 * -keyalg指定算法，这里是RSA 
 * -keystore指定存储位置，这里是d:\zlex.keystore 
 * 
 * 在这里我使用的密码为 123456 
 * 
 * 2.生成自签名证书 光有keyStore文件是不够的，还需要证书文件，证书才是直接提供给外界使用的公钥凭证。 
 * 导出证书：
 * 		keytool -export -keystore hcy.keystore -alias www.hcy.org -file hcy.cer -rfc
 * 
 * 其中 
 * -export指定为导出操作 
 * -keystore指定keystore文件 
 * -alias指定导出keystore文件中的别名 
 * -file指向导出路径 
 * -rfc以文本格式输出，也就是以BASE64编码输出 
 * 
 * 这里的密码是 123456 
 */
public class CertificateCoderTest
{
	private String password = "123456";
	private String alias = "www.hcy.org";
	private String certificatePath = "/home/hcy/hcy.cer";
	private String keyStorePath = "/home/hcy/hcy.keystore";
	
	public void test() throws Exception
	{
		System.out.println("公钥加密——私钥解密");
		String inputStr = "黄丛宇06161032";
		byte[] data = inputStr.getBytes();
		
		byte[] encrypt = CertificateCoder.encryptByPublicKey(data, certificatePath);
		BigInteger endata = new BigInteger(encrypt);
		System.out.println("加密后：" + endata.toString(16));
		byte[] decrypt = CertificateCoder.decryptByPrivateKey(encrypt, keyStorePath, alias, password);
		String outputStr = new String(decrypt);
		
		System.out.println("加密前: " + inputStr + "\n\r" + "解密后: " + outputStr);
		
		// 验证证书有效
		CertificateCoder.verifyCertificate(certificatePath);
		
	}
	
	public void testSign() throws Exception
	{
		System.out.println("私钥加密——公钥解密");
		
		String inputStr = "黄丛宇06161032";
		byte[] data = inputStr.getBytes();
		
		byte[] encodedData = CertificateCoder.encryptByPrivateKey(data, keyStorePath, alias, password);
		BigInteger endata = new BigInteger(encodedData);
		System.out.println("加密后：" + endata.toString(16));
		byte[] decodedData = CertificateCoder.decryptByPublicKey(encodedData, certificatePath);
		
		String outputStr = new String(decodedData);
		System.out.println("加密前: " + inputStr + "\n\r" + "解密后: " + outputStr);
		
		System.out.println("私钥签名——公钥验证签名");
		// 产生签名
		String sign = CertificateCoder.sign(encodedData, keyStorePath, alias, password);
		System.out.println("签名:\r" + sign);
		
		// 验证签名
		boolean status = CertificateCoder.verify(encodedData, sign, certificatePath);
		System.out.println("状态:\r" + status);
		
	}
}
