package edu.xjtu.se.hcy.secondencrypt;

import java.security.Key;
import java.security.KeyFactory;
import java.security.KeyPair;
import java.security.KeyPairGenerator;
import java.security.NoSuchAlgorithmException;
import java.security.PrivateKey;
import java.security.PublicKey;
import java.security.Signature;
import java.security.interfaces.RSAPrivateKey;
import java.security.interfaces.RSAPublicKey;
import java.security.spec.PKCS8EncodedKeySpec;
import java.security.spec.X509EncodedKeySpec;

import java.util.HashMap;
import java.util.Map;

import javax.crypto.Cipher;

public class RSACoder extends Coder
{
	private byte[] publicKey = null;
	private byte[] privateKey = null;
	
	/**
	 * 用私钥对信息生成数字签名
	 * 
	 * @param data 加密数据
	 * @param privateKey 私钥
	 * @return
	 * @throws Exception
	 */
	public byte[] sign(byte[] data, byte[] privateKey) throws Exception
	{
		byte[] keyBytes = privateKey;
		
		// 构造PKCS8EncodedKeySpec对象
		PKCS8EncodedKeySpec pkcs8KeySpec = new PKCS8EncodedKeySpec(keyBytes);
		
		// KEY_ALGORITHM 指定的加密算法
		KeyFactory keyFactory = KeyFactory.getInstance("RSA");
		
		// 取私钥匙对象
		PrivateKey priKey = keyFactory.generatePrivate(pkcs8KeySpec);
		
		// 用私钥对信息生成数字签名
		Signature signature = Signature.getInstance("MD5withRSA");
		signature.initSign(priKey);
		signature.update(data);
		
		return signature.sign();
	}
	
	/**
	 * 校验数字签名
	 * 
	 * @param data 加密数据
	 * @param publicKey 公钥
	 * @param sign 数字签名
	 * @return 校验成功返回true 失败返回false
	 * @throws Exception
	 * 
	 */
	public boolean verify(byte[] data, byte[] publicKey, byte[] sign) throws Exception
	{
		
		byte[] keyBytes = publicKey;
		
		// 构造X509EncodedKeySpec对象
		X509EncodedKeySpec keySpec = new X509EncodedKeySpec(keyBytes);
		
		// KEY_ALGORITHM 指定的加密算法
		KeyFactory keyFactory = KeyFactory.getInstance("RSA");
		
		// 取公钥匙对象
		PublicKey pubKey = keyFactory.generatePublic(keySpec);
		
		Signature signature = Signature.getInstance("MD5withRSA");
		signature.initVerify(pubKey);
		signature.update(data);
		
		// 验证签名是否正常
		return signature.verify(sign);
	}
	
	/**
	 * 解密<br>
	 * 用私钥解密
	 * 
	 * @param data
	 * @param key
	 * @return
	 * @throws Exception
	 */
	public byte[] decryptByPrivateKey(byte[] data, byte[] key) throws Exception
	{
		// 对密钥解密
		byte[] keyBytes = key;
		
		// 取得私钥
		PKCS8EncodedKeySpec pkcs8KeySpec = new PKCS8EncodedKeySpec(keyBytes);
		KeyFactory keyFactory = KeyFactory.getInstance("RSA");
		Key privateKey = keyFactory.generatePrivate(pkcs8KeySpec);
		
		// 对数据解密
		Cipher cipher = Cipher.getInstance(keyFactory.getAlgorithm());
		cipher.init(Cipher.DECRYPT_MODE, privateKey);
		
		return cipher.doFinal(data);
	}
	
	/**
	 * 解密<br>
	 * 用公钥解密
	 * 
	 * @param data
	 * @param key
	 * @return
	 * @throws Exception
	 */
	public byte[] decryptByPublicKey(byte[] data, byte[] key) throws Exception
	{
		// 对密钥解密
		byte[] keyBytes = key;
		
		// 取得公钥
		X509EncodedKeySpec x509KeySpec = new X509EncodedKeySpec(keyBytes);
		KeyFactory keyFactory = KeyFactory.getInstance("RSA");
		Key publicKey = keyFactory.generatePublic(x509KeySpec);
		
		// 对数据解密
		Cipher cipher = Cipher.getInstance(keyFactory.getAlgorithm());
		cipher.init(Cipher.DECRYPT_MODE, publicKey);
		
		return cipher.doFinal(data);
	}
	
	/**
	 * 加密<br>
	 * 用公钥加密
	 * 
	 * @param data
	 * @param key
	 * @return
	 * @throws Exception
	 */
	public byte[] encryptByPublicKey(byte[] data, byte[] key) throws Exception
	{
		// 对公钥解密
		byte[] keyBytes = key;
		
		// 取得公钥
		X509EncodedKeySpec x509KeySpec = new X509EncodedKeySpec(keyBytes);
		KeyFactory keyFactory = KeyFactory.getInstance("RSA");
		Key publicKey = keyFactory.generatePublic(x509KeySpec);
		
		// 对数据加密
		Cipher cipher = Cipher.getInstance(keyFactory.getAlgorithm());
		cipher.init(Cipher.ENCRYPT_MODE, publicKey);
		
		return cipher.doFinal(data);
	}
	
	/**
	 * 加密<br>
	 * 用私钥加密
	 * 
	 * @param data
	 * @param key
	 * @return
	 * @throws Exception
	 */
	public byte[] encryptByPrivateKey(byte[] data, byte[] key) throws Exception
	{
		// 对密钥解密
		byte[] keyBytes = key;
		
		// 取得私钥
		PKCS8EncodedKeySpec pkcs8KeySpec = new PKCS8EncodedKeySpec(keyBytes);
		KeyFactory keyFactory = KeyFactory.getInstance("RSA");
		Key privateKey = keyFactory.generatePrivate(pkcs8KeySpec);
		
		// 对数据加密
		Cipher cipher = Cipher.getInstance(keyFactory.getAlgorithm());
		cipher.init(Cipher.ENCRYPT_MODE, privateKey);
		
		return cipher.doFinal(data);
	}
	
	/**
	 * 取得私钥
	 */
	public byte[] getPrivateKey() 
	{
		return privateKey;
	}
	
	/**
	 * 取得公钥
	 */
	public byte[] getPublicKey()
	{
		return publicKey;
	}
	
	/**
	 * 初始化密钥
	 * 
	 * @return
	 * @throws NoSuchAlgorithmException 
	 */
	public void initKey() throws NoSuchAlgorithmException
	{
		KeyPairGenerator keyPairGen = KeyPairGenerator.getInstance("RSA");
		keyPairGen.initialize(1024);
		KeyPair keyPair = keyPairGen.generateKeyPair();
		
		// 公钥
		RSAPublicKey rsapublickey = (RSAPublicKey) keyPair.getPublic();
		
		// 私钥
		RSAPrivateKey rsaprivatekey = (RSAPrivateKey) keyPair.getPrivate();
		
		publicKey = rsapublickey.getEncoded();
		privateKey = rsaprivatekey.getEncoded();
		
	}
}
