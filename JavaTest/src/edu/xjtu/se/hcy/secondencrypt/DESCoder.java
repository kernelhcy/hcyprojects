package edu.xjtu.se.hcy.secondencrypt;

import java.security.Key;
import java.security.SecureRandom;
import java.util.Random;

import javax.crypto.Cipher;
import javax.crypto.KeyGenerator;
import javax.crypto.SecretKey;
import javax.crypto.SecretKeyFactory;
import javax.crypto.spec.DESKeySpec;
import javax.crypto.spec.IvParameterSpec;

/**
 * DES安全编码组件
 * 
 * <pre>
 * 支持 DES、DESede(TripleDES,就是3DES)、AES、Blowfish、RC2、RC4(ARCFOUR)
 * DES          		key size must be equal to 56
 * DESede(TripleDES) 	key size must be equal to 112 or 168
 * AES          		key size must be equal to 128, 192 or 256,but 192 and 256 bits may not be available
 * Blowfish     		key size must be multiple of 8, and can only range from 32 to 448 (inclusive)
 * RC2          		key size must be between 40 and 1024 bits
 * RC4(ARCFOUR) 		key size must be between 40 and 1024 bits
 * 具体内容 需要关注 JDK Document http://.../docs/technotes/guides/security/SunProviders.html
 * </pre>
 * 
 * DES-Data Encryption Standard,即数据加密算法。是IBM公司于1975年研究成功并公开发表的。
 * DES算法的入口参数有三个:Key、Data、Mode。其中Key为8个字节共64位,是DES算法的工作密钥;Data也为8个字节64位,
 * 是要被加密或被解密的数据;Mode为DES的工作方式,有两种:加密或解密。 
 * DES算法把64位的明文输入块变为64位的密文输出块,它所使用的密钥也是64位。 
 */
public class DESCoder// extends Coder
{	
	/*
	 * DES算法的不同应用方式
	 * CFB和OFB可以设置块大小
	 */
	public static final int MODE_ECB = 1;
	public static final int MODE_CBC = 2;
	public static final int MODE_CFB = 3;
	public static final int MODE_OFB = 4;
	public static final int MODE_CTR = 5;
	
	/*
	 * 消息填充模式。
	 * 两种模式：不填充和PKCS5填充模式
	 */
	public static final int PAD_NO = 1;
	public static final int PAD_PKCS5 = 2;

	
	//CBC, CFB, OFB模式下的初始化向量 
	private byte[] VI = null;
	private IvParameterSpec ips = null;
	//应用模式.ECB, CBC, CFB, OFB
	private int mode = 0;
	//消息填充模式
	private int padding = 0;
	//cipher类的模式
	private String transformation = "DES";
	
	/**
	 * 只设定应用模式
	 * @param mode
	 */
	public DESCoder(int mode)
	{
		this(mode, null, PAD_PKCS5);
	}
	
	/**
	 * 设定应用模式，初始化向量和填充模式
	 * @param mode
	 * @param vi
	 * @param pad
	 */
	public DESCoder(int mode, byte[] vi, int pad)
	{
		this.mode = mode;
		if (null == vi)
		{
			this.VI = new byte[8];
			Random rand = new Random();
			for (int i = 0; i < 8; ++i)
			{
				this.VI[i] = (byte)rand.nextInt();
			}
		}
		else
		{
			this.VI = vi;
		}
		this.padding = pad;
		this.ips = new IvParameterSpec(VI);
		
		switch (mode)
		{
			case MODE_CBC:
				transformation += "/CBC";
				break;
			case MODE_ECB:
				transformation += "/ECB";
				break;
			case MODE_CFB:
				transformation += "/CFB";
				break;
			case MODE_OFB:
				transformation += "/OFB";
				break;
			case MODE_CTR:
				transformation += "/CTR";
				break;
			default:
				System.err.println("Unknown mode.");
				break;
		}
		
		switch (padding)
		{
			case PAD_NO:
				transformation += "/NoPadding";
				break;
			case PAD_PKCS5:
				transformation += "/PKCS5Padding";
				break;
			default:
				System.err.println("Unknown padding mode.");
				break;
		}
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
		
		// 当使用其他对称加密算法时，如AES、Blowfish等算法时，用下述代码替换上述三行代码
		// SecretKey secretKey = new SecretKeySpec(key, ALGORITHM);
		
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
		Cipher cipher = Cipher.getInstance(transformation);
		if (mode == MODE_ECB)
		{
			cipher.init(Cipher.DECRYPT_MODE, k);
		}
		else 
		{
			cipher.init(Cipher.DECRYPT_MODE, k, ips);
		}
		
		
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
		System.out.println("transformation: " + transformation);
		Key k = toKey(key);
		Cipher cipher = Cipher.getInstance(transformation);
		if (mode == MODE_ECB)
		{
			cipher.init(Cipher.ENCRYPT_MODE, k);
		}
		else 
		{
			cipher.init(Cipher.ENCRYPT_MODE, k, ips);
		}
		
//		int blocksize = cipher.getBlockSize();
//		System.out.println("DES 块大小为:" + blocksize);
		
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
