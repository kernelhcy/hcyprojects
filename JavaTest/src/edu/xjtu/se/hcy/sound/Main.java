/**
 * 
 */
package edu.xjtu.se.hcy.sound;

import java.applet.Applet;
import java.applet.AudioClip;
import java.net.MalformedURLException;
import java.net.URL;


public class Main {
	
	public static void main(String args[]) {
		
		URL path=null;
		AudioClip ac=null;
		
		try {
			path=new URL("file://home/hcy/桌面/千里之外.aiff");
		} catch (MalformedURLException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
		System.out.println(path.getFile());
		ac=Applet.newAudioClip(path);
		
		ac.play();
		//ac.stop();
		ac.loop();
	}
}