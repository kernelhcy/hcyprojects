package edu.xjtu.se.hcy.javamail;

import java.util.*;
import javax.mail.*;
import javax.mail.internet.*;
import javax.activation.*;

public class JavaMail {

	private MimeMessage mimeMsg; // MIME邮件对象

	private Session session; // 邮件会话对象

	private Properties props; // 系统属性

//	private boolean needAuth = false; // smtp是否需要认证

	private String username = null; // smtp认证用户名和密码

	private String password = null;

	private Multipart mp; // Multipart对象,邮件内容,标题,附件等内容均添加到其中后再生成MimeMessage对象

	public JavaMail(String smtp) {

		setSmtpHost(smtp);

		createMimeMessage();

	}

	/**
	 * 
	 * @param hostName
	 *            String
	 */

	public void setSmtpHost(String hostName) {

		System.out.println("设置系统属性：mail.smtp.host = " + hostName);

		if (props == null) {
			props = System.getProperties(); // 获得系统属性对象
		}

		props.put("mail.smtp.host", hostName); // 设置SMTP主机

	}

	/**
	 * 
	 * @return boolean
	 */

	public boolean createMimeMessage() {

		try {

			System.out.println("准备获取邮件会话对象！");

			session = Session.getDefaultInstance(props, null); // 获得邮件会话对象

		} catch (Exception e) {

			System.err.println("获取邮件会话对象时发生错误！" + e);

			return false;

		}

		System.out.println("准备创建MIME邮件对象！");

		try {

			mimeMsg = new MimeMessage(session); // 创建MIME邮件对象

			mp = new MimeMultipart();

			return true;

		} catch (Exception e) {

			System.err.println("创建MIME邮件对象失败！" + e);

			return false;

		}

	}

	/**
	 * 
	 * @param need
	 *            boolean
	 */

	public void setNeedAuth(boolean need) {

		System.out.println("设置smtp身份认证：mail.smtp.auth = " + need);

		if (props == null) {
			props = System.getProperties();
		}

		if (need) {

			props.put("mail.smtp.auth", "true");

		} else {

			props.put("mail.smtp.auth", "false");

		}

	}

	/**
	 * 
	 * @param name
	 *            String
	 * 
	 * @param pass
	 *            String
	 */

	public void setNamePass(String name, String pass) {

		username = name;

		password = pass;

	}

	/**
	 * 
	 * @param mailSubject
	 *            String
	 * 
	 * @return boolean
	 */

	public boolean setSubject(String mailSubject) {

		System.out.println("设置邮件主题！");

		try {

			mimeMsg.setSubject(mailSubject);

			return true;

		} catch (Exception e) {

			System.err.println("设置邮件主题发生错误！");

			return false;

		}

	}

	/**
	 * 
	 * @param mailBody
	 *            String
	 */

	public boolean setBody(String mailBody) {

		try {

			BodyPart bp = new MimeBodyPart();

			mp.removeBodyPart(bp);

			bp.setContent("" + mailBody, "text/html;charset=GB2312");

			mp.addBodyPart(bp);

			return true;

		} catch (Exception e) {

			System.err.println("设置邮件正文时发生错误！" + e);

			return false;

		}

	}

	/**
	 * 
	 * @param name
	 *            String
	 * 
	 * @param pass
	 *            String
	 */

	public boolean addFileAffix(String filename) {

		System.out.println("增加邮件附件：" + filename);

		try {

			BodyPart bp = new MimeBodyPart();

			FileDataSource fileds = new FileDataSource(filename);

			bp.setDataHandler(new DataHandler(fileds));

			bp.setFileName(fileds.getName());

			mp.addBodyPart(bp);

			return true;

		} catch (Exception e) {

			System.err.println("增加邮件附件：" + filename + "发生错误！" + e);

			return false;

		}

	}

	/**
	 * 
	 * @param name
	 *            String
	 * 
	 * @param pass
	 *            String
	 */

	public boolean setFrom(String from) {

		System.out.println("设置发信人！");

		try {

			mimeMsg.setFrom(new InternetAddress(from)); // 设置发信人

			return true;

		} catch (Exception e) {
			return false;
		}

	}

	/**
	 * 
	 * @param name
	 *            String
	 * 
	 * @param pass
	 *            String
	 */

	public boolean setTo(String to) {

		if (to == null) {
			return false;
		}

		try {

			mimeMsg.setRecipients(Message.RecipientType.TO, InternetAddress
					.parse(to));

			return true;

		} catch (Exception e) {
			return false;
		}

	}

	/**
	 * 转发
	 * @param name
	 *            String
	 * 
	 * @param pass
	 *            String
	 */

	public boolean setCopyTo(String copyto) {

		if (copyto == null) {
			return false;
		}

		try {

			mimeMsg.setRecipients(Message.RecipientType.CC,
					(Address[]) InternetAddress.parse(copyto));

			return true;

		} catch (Exception e) {
			return false;
		}

	}

	/**
	 * 只发送一封
	 * 
	 * @return
	 */
	public boolean sendout() {
		return sendout(1);
	}

	/**
	 * 重复发送n封
	 * 
	 * @param n
	 * @return
	 */
	public boolean sendout(int n) {

		try {

			mimeMsg.setContent(mp);

			mimeMsg.saveChanges();
			
//			System.out.println("正在发送邮件....");

			Session mailSession = Session.getInstance(props, null);

			Transport transport = mailSession.getTransport("smtp");

			transport.connect((String) props.get("mail.smtp.host"), username,
					password);

			for (int i = 0; i < n; i++) {
				System.out.print("发送第"+(i+1)+"封....");
				transport.sendMessage(mimeMsg, mimeMsg
						.getRecipients(Message.RecipientType.TO));
				System.out.println("完成！");
			}
			
			transport.close();

			System.out.println("共发送"+n+"封。");
			return true;

		} catch (Exception e) {

			System.err.println("邮件发送失败！" + e);

			return false;

		}

	}

	/**
	 * 
	 * Just do it as this
	 */

	public static void main(String[] args) {

		String mailbody = "实验<br>Just test the java mail...";
		String affixfilename="/home/hcy/workspace/test/lib/mail.jar";
		String subject="邮件主题";
		String fromURL="huangcongyu2006@qq.com";
		String toURL="huangcongyu2006@gmail.com";
		String user="huangcongyu2006";
		String pwd="hcyilyzj";
		
		
		JavaMail mail = new JavaMail("smtp.qq.com");
		mail.setNeedAuth(false);

		if (mail.setFrom(fromURL) == false) {
			System.err.println("设置发信人失败。。。");
			return;
		}

		if (mail.setTo(toURL) == false) {
			System.err.println("设置收信人失败。。。");
			return;
		}

		mail.setNamePass(user, pwd);

		if (mail.setSubject(subject) == false) {
			System.err.println("设置标题失败。。。");
			return;
		}

		if (mail.setBody(mailbody) == false) {
			System.err.println("设置信件内容失败。。。");
			return;
		}
		
		if(mail.addFileAffix(affixfilename)==false){
			System.err.println("添加附件失败。。。");
			return ;
		}
		if (mail.sendout(5) == false) {
			System.err.println("发送失败。。。");
			return;
		}

	}
}