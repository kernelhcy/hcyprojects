package problem7;

import java.awt.Color;
import java.awt.Cursor;
import java.awt.Dimension;
import java.awt.Graphics;
import java.awt.Graphics2D;
import java.awt.event.MouseAdapter;
import java.awt.event.MouseEvent;
import java.awt.event.MouseListener;
import java.awt.event.MouseMotionAdapter;
import java.util.ArrayList;
import javax.swing.JPanel;
/**
 * 画图面板。
 * 
 * JDK中提供了各种图形的封装类。所有类都实现了Shape接口，
 * 在面板中画这些图形，只需要g2d.draw(s)即可。
 * @author hcy
 *
 */
public class DrawPanel extends JPanel
{
	
	public DrawPanel() {
		Shapes = new ArrayList<MyShape>();
		cll = new CreateLineListener(this);
		crl = new CreateRectListener(this);
		currl = null;
		this.addMouseMotionListener(new MouseMotionAdapter()
		{			
			@Override
			public void mouseDragged(MouseEvent e)
			{
				int w = -1,h = -1;
				// TODO Auto-generated method stub
				if(e.getX() > dp.getWidth()){
					w = e.getX() + 10;
				}
				if(e.getY() > dp.getHeight()){
					h = e.getY() + 10;
				}
				
				w = (w == -1) ? dp.getWidth() : w;
				h = (h == -1) ? dp.getHeight() : h;
				
				//必须同时调用这两个函数才能是ScrollPanel捕获到
				//Panel的大小发生了变化。
				dp.setPreferredSize(new Dimension(w, h));
				dp.setSize(w, h);
				
			}
		});
	}

	/**
	 * 根据所需要画的图形的类型，设置监听器。
	 * @param st
	 */
	public void setShapeListener(ShapeType st, boolean fill, Color c)
	{
		//移出旧的监听器。
		if(currl != null){
			this.setCursor(new Cursor(Cursor.DEFAULT_CURSOR));
			this.removeMouseListener(currl);
			this.removeMouseMotionListener(currl);
		}
		switch(st){
		case RECT:
			this.setCursor(new Cursor(Cursor.CROSSHAIR_CURSOR));
			crl.setColor(c);
			crl.setFill(fill);
			this.addMouseListener(crl);
			this.addMouseMotionListener(crl);
			currl = crl;
			break;
		case LINE:
			this.setCursor(new Cursor(Cursor.CROSSHAIR_CURSOR));
			cll.setColor(c);
			cll.setFill(fill);
			this.addMouseListener(cll);
			this.addMouseMotionListener(cll);
			currl = cll;
			break;
		case CIRCLE:
			this.setCursor(new Cursor(Cursor.CROSSHAIR_CURSOR));
			break;
		case NONE: 	//删除监听器。
			break;
		default:
			break;
		}
	}
	
	/**
	 * 擦除所有形状。
	 */
	public void clear()
	{
		Shapes.clear();
		this.repaint();
	}
	
	public void setColor(Color c)
	{
		if(currl != null){
			currl.setColor(c);
		}
		
	}
	
	public void setFill(boolean fill)
	{
		if(currl != null){
			currl.setFill(fill);
		}
		
	}
	
	public void addSharp(MyShape s)
	{
		Shapes.add(s);

	}

	public MyShape deleteSharp(MyShape s)
	{
		Shapes.remove(s);
		return s;
	}

	public void paint(Graphics g)
	{
		Graphics2D g2d = (Graphics2D)g;
		//clear
		g2d.setColor(Color.WHITE);
		g2d.fillRect(0, 0, this.getWidth() + 1, this.getHeight() + 1);
		g2d.setColor(Color.BLACK);
		
		Color c = null, oldc = null;
		for (MyShape s : Shapes) {
			oldc = g2d.getColor();
			c = s.getColor();
			g2d.setColor(c);
			if(s.isFill()){
				g2d.fill(s.getShape());
			}else{
				g2d.draw(s.getShape());
			}
			
			g2d.setColor(oldc);
		}
	}
	
	private ArrayList<MyShape> Shapes;
	private MyMouseAdapter cll; //画直线的监听器
	private MyMouseAdapter crl; //画矩形的监听器
	
	private MyMouseAdapter currl;  //当前鼠标事件监听器。
	
	private DrawPanel dp = this;
}
