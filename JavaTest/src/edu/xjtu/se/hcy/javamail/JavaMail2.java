package edu.xjtu.se.hcy.javamail;

import javax.mail.*;
import javax.mail.internet.*;
import java.util.*;

public class JavaMail2 {
	public static void main(String args[]) throws Exception {

		String host = "stu.xjtu.edu.cn";
		String from = "huangcongyu2006@stu.xjtu.edu.cn";
		String to = "huangcongyu2006@gmail.com";
		String username = "huangcongyu2006";
		String password = "hcyilyzj";

		// Get system properties
		// Properties props = System.getProperties();
		// 很多例子中是这样的，其实下面这句更好，可以用在applet中
		Properties props = new Properties();

		// Setup mail server
		props.put("mail.smtp.host", host);
		props.put("mail.smtp.auth", "false"); // 这样才能通过验证

		// Get session
		Session session = Session.getDefaultInstance(props);

		// watch the mail commands go by to the mail server
		session.setDebug(false);

		// Define message
		MimeMessage message = new MimeMessage(session);
		message.setFrom(new InternetAddress(from));
		message.addRecipient(Message.RecipientType.TO, new InternetAddress(to));
		
		System.out.println("设置主题");
		message.setSubject("Hello JavaMail");
		
		System.out.println("设置信件内容");
		message.setText("Welcome to JavaMail");

		// Send message
		System.out.println("发送邮件。。。");
		message.saveChanges();
		Transport transport = session.getTransport("smtp");
		transport.connect(host, username, password);
		transport.sendMessage(message, message.getAllRecipients());
		transport.close();
		System.out.println("发送成功！");
	}
}