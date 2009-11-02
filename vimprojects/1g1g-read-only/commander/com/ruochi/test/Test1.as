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
	public class Test1 extends Sprite {
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
		
		public function Test1() {
			
			draw();
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
			graphics.clear();
			graphics.lineStyle(1, 0);
			graphics.curveTo(100, 100, 120, 40);
			
			graphics.moveTo(50, 50);
			graphics.lineTo(110,70)
			
	
			
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