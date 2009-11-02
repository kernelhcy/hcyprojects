package com.ruochi.test {
	import com.ruochi.shape.player.PauseShape;
	import flash.display.*;
	import com.ruochi.geom.*;
	import flash.events.*;
	import flash.geom.*;
	import com.ruochi.utils.*;
	import com.ruochi.shape.*;
	import Singularity.Geom.*
	import Singularity.Geom.Ellipse
	public class Test extends Sprite {
		private var _line:Line = new Line(new Point(0, 0), new Point(2, 2))
		private var _angle:Angle = new Angle(60);
		private var p1:Point = new Point(0, 0);
		private var p2:Point = new Point(140, 70);
		private var p3:Point = new Point(550, 0);
		private var cp1:Point = new Point();
		private var cp2:Point = new Point();
		private var ps1:PointShape = new PointShape
		private var ps2:PointShape = new PointShape
		private var ps3:PointShape = new PointShape
		
		private var cps1:PointShape = new PointShape
		private var cps2:PointShape = new PointShape
		
		var b:Bezier2 = new Bezier2()
		var c:Ellipse = new Ellipse();
		
		var shape:Shape = new Shape();
		
		public function Test() {
			
			c.container = shape
			addChild(shape);
			stage.addEventListener(MouseEvent.CLICK , onStageClick, false, 0, true);
			addChild(ps1);
			addChild(ps2);
			addChild(ps3);
			addChild(cps1);
			addChild(cps2);
		}
		
		private function onStageClick(e:MouseEvent):void {
			draw()
		}
		
		private function draw():void {
			trace(Math.atan(1/Math.sqrt(3))/Math.PI*180);
			graphics.clear();
			graphics.lineStyle(1, 0);
			var line0:Line = new Line(p1, p3);
			
			var line1:Line = new Line();
			var angle0:Angle = new Angle();
			angle0.radian = Math.atan(line0.k);
			line1.setPointAngle(p2, angle0);
			
			
			var line2:Line = new Line();
			var angle1:Angle = new Angle();
			angle1.radian = Math.atan((p2.y - p1.y) / (p2.x - p1.x));
			var angle2:Angle = new Angle();
			angle2.radian =  2 * angle1.radian -angle0.radian;
			line2.setPointAngle(p1, angle2);
			cp1 = Line.getIntersection(line1, line2);
			
			
			var line3:Line = new Line();
			var angle3:Angle = new Angle();
			angle3.radian = Math.atan((p3.y - p2.y) / (p3.x - p2.x));
			var angle4:Angle = new Angle();
			angle4.radian =  2 * angle3.radian -angle0.radian;
			line3.setPointAngle(p3, angle4);
			cp2 = Line.getIntersection(line1, line3);
			
			
			
	
			
			/*graphics.moveTo(p1.x, p1.y);
			graphics.lineTo(p3.x, p3.y);
			
			graphics.moveTo(p1.x, p1.y);
			graphics.lineTo(p2.x, p2.y);
			
			graphics.moveTo(p2.x, p2.y);
			graphics.lineTo(0, line1.getY(0));
			
			graphics.moveTo(p1.x, p1.y);
			graphics.lineTo(300, line2.getY(300));*/
			
			graphics.moveTo(p1.x, p1.y);
			graphics.curveTo(cp1.x, cp1.y, p2.x, p2.y);
			graphics.moveTo(p2.x, p2.y);
			graphics.curveTo(cp2.x, cp2.y, p3.x, p3.y);
	
			
			ps1.x = p1.x;
			ps1.y = p1.y
			ps2.x = p2.x;
			ps2.y = p2.y;
			ps3.x = p3.x;
			ps3.y = p3.y;
			cps1.x = cp1.x;
			cps1.y = cp1.y; 
			cps2.x = cp2.x;
			cps2.y = cp2.y; 
		}
	}
}