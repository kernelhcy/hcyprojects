package edu.xjtu.se.hcy.encrypt;
import java.math.BigInteger;
import java.util.Map;

/**
 * 
 * @author 梁栋
 * @version 1.0
 * @since 1.0
 * 
 * 流程分析： 
 * 1.甲方构建密钥对儿，将公钥公布给乙方，将私钥保留；双方约定数据加密算法；
 * 乙方通过甲方公钥构建密钥对儿，将公钥公布给甲方，将私钥保留。 
 * 2.甲方使用私钥、乙方公钥、约定数据加密算法构建本地密钥，然后通过本地密钥加密数据，发送给乙方加密后的数据；
 * 乙方使用私钥、甲方公钥、约定数据加密算法构建本地密钥，然后通过本地密钥对数据解密。 
 * 3.乙方使用私钥、甲方公钥、约定数据加密算法构建本地密钥，然后通过本地密钥加密数据，发送给甲方加密后的数据；
 * 甲方使用私钥、乙方公钥、约定数据加密算法构建本地密钥，然后通过本地密钥对数据解密。 
 */
public class DHCoderTest
{
	public void test() throws Exception
	{
		// 生成甲方密钥对儿
		Map<String, Object> aKeyMap = DHCoder.initKey();
		String aPublicKey = DHCoder.getPublicKey(aKeyMap);
		String aPrivateKey = DHCoder.getPrivateKey(aKeyMap);
		
		System.out.println("甲方公钥:\r" + aPublicKey);
		System.out.println("甲方私钥:\r" + aPrivateKey);
		
		// 由甲方公钥产生本地密钥对儿
		Map<String, Object> bKeyMap = DHCoder.initKey(aPublicKey);
		String bPublicKey = DHCoder.getPublicKey(bKeyMap);
		String bPrivateKey = DHCoder.getPrivateKey(bKeyMap);
		
		System.out.println("乙方公钥:\r" + bPublicKey);
		System.out.println("乙方私钥:\r" + bPrivateKey);
		
		BigInteger encryptedData ;
		
		String aInput = "黄丛宇06161032";
		System.out.println("原文: " + aInput);
		
		// 由甲方公钥，乙方私钥构建密文
		byte[] aCode = DHCoder.encrypt(aInput.getBytes(), aPublicKey, bPrivateKey);
		encryptedData = new BigInteger(aCode);
		System.out.println("加密后:" + encryptedData.toString(16));
		
		// 由乙方公钥，甲方私钥解密
		byte[] aDecode = DHCoder.decrypt(aCode, bPublicKey, aPrivateKey);
		String aOutput = (new String(aDecode));
		System.out.println("解密: " + aOutput);
		
		System.out.println(" ===============反过来加密解密================== ");
		String bInput = "黄丛宇06161032";
		System.out.println("原文: " + bInput);
		
		// 由乙方公钥，甲方私钥构建密文
		byte[] bCode = DHCoder.encrypt(bInput.getBytes(), bPublicKey, aPrivateKey);
		encryptedData = new BigInteger(aCode);
		System.out.println("加密后:" + encryptedData.toString(16));
		
		// 由甲方公钥，乙方私钥解密
		byte[] bDecode = DHCoder.decrypt(bCode, aPublicKey, bPrivateKey);
		String bOutput = (new String(bDecode));
		System.out.println("解密: " + bOutput);
	}
	
}
