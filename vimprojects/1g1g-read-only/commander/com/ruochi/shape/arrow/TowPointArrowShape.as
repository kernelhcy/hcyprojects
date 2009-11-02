package com.ruochi.shape.arrow {
	import com.ruochi.geom.Angle;
	import flash.display.Shape;
	import flash.geom.Point;
	public class TowPointArrowShape extends Shape {
		private var _color:uint;
		private var _p1:Point;
		private var _p2:Point;
		public function TowPointArrowShape(p1:Point, p2:Point, color:uint = 0xffffff) {
			_p1 = p1;
			_p2 = p2;
			_color = color;
			draw()
		}
		private function draw():void {
			var mainLineAngle:Angle = Angle.PoiotToAngel(_p2, _p1);
			var mainDistance:Number = Point.distance(_p1, _p2); 
			var p3:Point = new Point();
			var p4:Point = new Point();
			p3.x = _p2.x - Math.cos(mainLineAngle.radian + Math.PI / 9) * mainDistance;
			p3.y = _p2.y - Math.sin(mainLineAngle.radian + Math.PI / 9) * mainDistance;
			p4.x = _p2.x - Math.cos(mainLineAngle.radian - Math.PI / 9) * mainDistance;
			p4.y = _p2.y - Math.sin(mainLineAngle.radian - Math.PI / 9) * mainDistance;
			graphics.clear();
			graphics.beginFill(_color);	
			graphics.moveTo(_p2.x, _p2.y);
			graphics.lineTo(p3.x, p3.y);
			//graphics.lineTo(100, 100);
			graphics.lineTo(p4.x, p4.y);
			//graphics.lineTo(_p2.x, _p2.y);
			graphics.endFill();
		}
		public function color(c:uint):void {
			_color = c;
			draw();
		}
	}
}