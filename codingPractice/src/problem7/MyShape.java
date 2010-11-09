package problem7;

import java.awt.Color;
import java.awt.Shape;
/**
 * 自定义形状。
 * 
 * 包含颜色和是否填充。
 * @author hcy
 *
 */
public interface MyShape
{
	Color getColor();
	Color setColor(Color c);
	boolean isFill();
	void setFill(boolean f);

}
