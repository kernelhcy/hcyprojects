package com.ruochi.shape{
	import flash.display.Shape;
	public class PointShape extends Shape {
		private var _r:Number;
		private var _color:uint;
		public function PointShape(r:Number = 2, c:uint = 0) {	
			_color = c;
			_r = r;
			buildUI();
		}
		public function buildUI() {
			graphics.clear();
			graphics.beginFill(_color);
			graphics.drawCircle(0, 0, _r);
			graphics.endFill();
		}
		public function set color(c:uint) {
			_color = c;
			buildUI();
		}
		public function get color():uint {
			return _color;
		}
		public function set r(r:Number) {
			_r = r;
			buildUI();
		}
		public function get r():Number {
			return _r;
		}
	}
}