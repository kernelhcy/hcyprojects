package edu.xjtu.se.hcy.test;

/**
 使用Java2D绘制五星红旗
 五星红旗的绘制标准见百度百科: http://baike.baidu.com/view/5163.htm
 注意: 该处对于第一颗小星的位置说明有误,应将“左五右十”改成“左十右五”
 @author  Eastsun
 @version 2008/10/17 1.0
 */
import javax.swing.*;
import java.awt.geom.*;
import java.awt.*;

public class NationalFlag extends JPanel {

	public static void main(String[] args) {
		JFrame frame = new JFrame("五星红旗 By Eastsun");
		frame.getContentPane().add(new NationalFlag(600));
		frame.pack();
		frame.setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);
		frame.setLocationRelativeTo(null);
		frame.setVisible(true);
	}

	/**
	 * 创建一个五角星形状. 该五角星的中心坐标为(sx,sy),中心到顶点的距离为radius,其中某个顶点与中心的连线的偏移角度为theta(弧度)
	 * 
	 * @return pentacle 一个☆
	 */
	public static Shape createPentacle(double sx, double sy, double radius,
			double theta) {
		final double arc = Math.PI / 5;
		final double rad = Math.sin(Math.PI / 10) / Math.sin(3 * Math.PI / 10);
		GeneralPath path = new GeneralPath();
		path.moveTo(1, 0);
		for (int idx = 0; idx < 5; idx++) {
			path.lineTo(rad * Math.cos((1 + 2 * idx) * arc), rad
					* Math.sin((1 + 2 * idx) * arc));
			path.lineTo(Math.cos(2 * (idx + 1) * arc), Math.sin(2 * (idx + 1)
					* arc));
		}
		path.closePath();
		AffineTransform atf = AffineTransform.getScaleInstance(radius, radius);
		atf.translate(sx / radius, sy / radius);
		atf.rotate(theta);
		return atf.createTransformedShape(path);
	}

	private int width, height;
	private double maxR = 0.15, minR = 0.05;
	private double maxX = 0.25, maxY = 0.25;
	private double[] minX = { 0.50, 0.60, 0.60, 0.50 };
	private double[] minY = { 0.10, 0.20, 0.35, 0.45 };

	/**
	 * 创建一个宽度为width的国旗
	 */
	public NationalFlag(int width) {
		this.width = width / 3 * 3;
		this.height = width / 3 * 2;
		setPreferredSize(new Dimension(this.width, this.height));
	}

	protected void paintComponent(Graphics g) {
		Graphics2D g2d = (Graphics2D) g;

		// 画旗面
		g2d.setPaint(Color.RED);
		g2d.fillRect(0, 0, width, height);

		// 画大☆
		double ox = height * maxX, oy = height * maxY;
		g2d.setPaint(Color.YELLOW);
		g2d.fill(createPentacle(ox, oy, height * maxR, -Math.PI / 2));

		// 画小★
		for (int idx = 0; idx < 4; idx++) {
			double sx = minX[idx] * height, sy = minY[idx] * height;
			double theta = Math.atan2(oy - sy, ox - sx);
			g2d.fill(createPentacle(sx, sy, height * minR, theta));
		}
	}
}