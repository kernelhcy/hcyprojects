package edu.xjtu.se.hcy.secondencrypt;

import java.security.Key;
import java.security.SecureRandom;
import javax.crypto.Cipher;
import javax.crypto.KeyGenerator;
import javax.crypto.SecretKey;
import javax.crypto.SecretKeyFactory;
import javax.crypto.spec.DESKeySpec;

public class DESCoder// extends Coder
{		
	/**
	 * 只设定应用模式
	 * @param mode
	 */
	public DESCoder()
	{
		
	}
	/**
	 * 转换密钥<br>
	 * 
	 * @param key
	 * @return
	 * @throws Exception
	 */
	private Key toKey(byte[] key) throws Exception
	{
		DESKeySpec dks = new DESKeySpec(key);
		SecretKeyFactory keyFactory = SecretKeyFactory.getInstance("DES");
		SecretKey secretKey = keyFactory.generateSecret(dks);
		
		return secretKey;
	}
	
	/**
	 * 解密
	 * 
	 * @param data
	 * @param key
	 * @return
	 * @throws Exception
	 */
	public byte[] decrypt(byte[] data, byte[] key) throws Exception
	{
		Key k = toKey(key);
		Cipher cipher = Cipher.getInstance("DES");
		cipher.init(Cipher.DECRYPT_MODE, k);
		
		return cipher.doFinal(data);
	}
	
	/**
	 * 加密
	 * 
	 * @param data
	 * @param key
	 * @return
	 * @throws Exception
	 */
	public byte[] encrypt(byte[] data, byte[] key) throws Exception
	{
		Key k = toKey(key);
		Cipher cipher = Cipher.getInstance("DES");
		cipher.init(Cipher.ENCRYPT_MODE, k);
		return cipher.doFinal(data);
	}
	
	/**
	 * 生成密钥
	 * 
	 * @return
	 * @throws Exception
	 */
	public byte[] initKey() throws Exception
	{
		return initKey(null);
	}
	
	/**
	 * 生成密钥
	 * 
	 * @param seed
	 * @return
	 * @throws Exception
	 */
	public byte[] initKey(String seed) throws Exception
	{
		SecureRandom secureRandom = null;
		
		if (seed != null)
		{
			secureRandom = new SecureRandom(seed.getBytes());
		}
		else
		{
			secureRandom = new SecureRandom();
		}
		
		KeyGenerator kg = KeyGenerator.getInstance("DES");
		kg.init(secureRandom);
		SecretKey secretKey = kg.generateKey();
		return secretKey.getEncoded();
	}
}
