package problem7;

import java.awt.Color;
import java.awt.Polygon;
import java.awt.event.MouseEvent;

public class CreatePolygonLintener extends MyMouseAdapter
{
	public CreatePolygonLintener(DrawPanel l){
		this.listenee = l;
		rightClicked = false;
	}
	
	@Override
	public void mouseMoved(MouseEvent e)
	{
		// TODO Auto-generated method stub
		if(!rightClicked && tmp != null){
			Polygon p = (Polygon)tmp.getShape();
			if(p.npoints == 2){
				/*
				 * 此时只有两个点。是一条直线。
				 * 直线不填充，否则显示不出来。
				 */
				if(fill){
					tmp.setFill(false);
				}
			}else{
				tmp.setFill(fill);
			}
			
			if(p.npoints != 1){
				/*
				 * 如果只有一个点，则不删除前面的点。
				 */
				--p.npoints;
			}
			p.addPoint(e.getX(), e.getY());
		}
		listenee.repaint();
	}

	@Override
	public void mouseClicked(MouseEvent e)
	{
		// TODO Auto-generated method stub
		if(tmp == null){
			tmp = new MyShape(ShapeType.POLYGON);
			tmp.setColor(c);
			tmp.setFill(fill);
			listenee.addShape(tmp);
			rightClicked = false;
		}
		
		if(e.getButton() == MouseEvent.BUTTON3){
			rightClicked = true;
			tmp = null;
		}else{
			Polygon p = (Polygon)tmp.getShape();
			p.addPoint(e.getX(), e.getY());
			System.out.println("Add Point: (" + e.getX()
					+ "," + e.getY() + ")");
			listenee.repaint();
		}
		
	}
	
	public void setColor(Color c)
	{
		this.c = c;
	}
	
	public void setFill(boolean fill)
	{
		
		this.fill = fill;
	}
	
	
	private DrawPanel listenee; // 被监听的对象。
	private MyShape tmp = null;;
	private boolean rightClicked; //鼠标右键被点击
	private boolean fill, oldfill;
	private Color c;
}
