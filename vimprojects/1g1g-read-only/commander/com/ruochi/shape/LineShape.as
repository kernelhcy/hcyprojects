package com.ruochi.shape{
	import com.ruochi.geom.Angle;
	import flash.display.Shape;
	import flash.geom.Point;
	public class LineShape extends Shape {
		private var _p1:Point;
		private var _p2:Point;
		private var _thikness:uint;
		private var _color:uint;
		public function LineShape(x1:Number=100, y1:Number=100, x2:Number=200, y2:Number=300, thikness:Number=20, c:uint=0xff0000) {	
			_p1 = new Point(x1, y1);
			_p2 = new Point(x2, y2);
			_thikness = thikness;
			_color = c;
			draw();
		}
		public function draw() {
			graphics.clear();
			var p1:Point = new Point();
			var p2:Point = new Point();
			var p3:Point = new Point();
			var p4:Point = new Point();
			var angle:Angle = new Angle(Math.PI / 2 - Angle.PoiotToAngel(_p1, _p2).radian);
			p1.x = _p1.x - _thikness / 2 * Math.cos(-angle.radian);
			p1.y = _p1.y - _thikness / 2 * Math.sin(-angle.radian);
			
			p2.x = _p1.x + _thikness / 2 * Math.cos(-angle.radian);
			p2.y = _p1.y + _thikness / 2 * Math.sin(-angle.radian);
			
			p3.x = _p2.x + _thikness / 2 * Math.cos(-angle.radian);
			p3.y = _p2.y + _thikness / 2 * Math.sin(-angle.radian);
			
			p4.x = _p2.x - _thikness / 2 * Math.cos(-angle.radian);
			p4.y = _p2.y - _thikness / 2 * Math.sin(-angle.radian);
			graphics.beginFill(_color);
			graphics.moveTo(p1.x, p1.y);
			graphics.lineTo(p2.x, p2.y);
			graphics.lineTo(p3.x, p3.y);
			graphics.lineTo(p4.x, p4.y);
			graphics.lineTo(p1.x, p1.y);
			graphics.endFill();
		}
		public function set color(c:uint) {
			_color = c;
			draw();
		}
		public function get color():uint {
			return _color;
		}
	}
}