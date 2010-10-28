package problem7;

import java.awt.Graphics;
import java.util.ArrayList;

import javax.swing.JPanel;

public class DrawPanel extends JPanel
{
	public DrawPanel()
	{
		sharps = new ArrayList<Sharp>();
	}
	
	public void addSharp(Sharp s)
	{
		sharps.add(s);
		
	}
	public Sharp deleteSharp(Sharp s)
	{
		sharps.remove(s);
		return s;
	}
	
	public void paint(Graphics g) 
	{
		//必须调用父类的pain函数！！
		super.paint(g);
		painSharps();
	}
	
	public void painSharps()
	{
		for(Sharp s : sharps){
			s.paintOn(this);
		}
	}
	
	private ArrayList<Sharp> sharps;
}
