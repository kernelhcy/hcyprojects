package edu.xjtu.se.hcy.encrypt;
import java.util.Map;


/**
 * 
 * @author 梁栋
 * @version 1.0
 * @since 1.0
 */
public class DSACoderTest
{
	
	public void test() throws Exception
	{
		String inputStr = "黄丛宇06161032";
		byte[] data = inputStr.getBytes();
		
		// 构建密钥
		Map<String, Object> keyMap = DSACoder.initKey();
		
		// 获得密钥
		byte[] publicKey = DSACoder.getPublicKey(keyMap);
		byte[] privateKey = DSACoder.getPrivateKey(keyMap);
		
		System.out.println("公钥:\r" + publicKey);
		System.out.println("私钥:\r" + privateKey);
		
		// 产生签名
		byte[] sign = DSACoder.sign(data, privateKey);
		System.out.println("签名:\r" + sign);
		
		// 验证签名
		boolean status = DSACoder.verify(data, publicKey, sign);
		System.out.println("状态:\r" + status);
		
	}
	
}
