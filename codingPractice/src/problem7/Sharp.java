package problem7;

import java.awt.Color;
import java.io.File;

import javax.swing.JComponent;
import javax.swing.JPanel;

/**
 * 定义形状的基本操作。
 * @author hcy
 *
 */
public abstract class Sharp
{
	/**
	 * 将这个形状绘制在panel上。
	 * 为了完整的显示形状，这个函数必须对panel的大小做合适的调整。
	 * @param panel
	 */
	public abstract void paintOn(JComponent panel);
	
	/*
	 * 将形状写入或从文件读入。
	 * off表示在文件中的偏移量。
	 */
	public abstract int writeToFile(File f, int off);
	public abstract int readFromFile(File f, int off);
	protected int writeHelp(File f, int off)
	{
		return 0;
	}
	
	protected int readHelp(File f, int off)
	{
		return 0;
	}
	/*
	 * 设置和读取颜色
	 */
	public abstract void setColor(Color c);
	public abstract Color getColor();
	
	/*
	 * 是否填充。
	 */
	public abstract void setFill(boolean isFill);
	public abstract boolean isFill();
}
