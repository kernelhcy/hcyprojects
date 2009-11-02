package com.ruochi.shape{
	import flash.display.Shape;
	public class Circle extends Shape {
		private var _r:Number;
		private var _color:uint;
		public function Circle(r:Number = 100, c:uint = 0xffffff) {			
			super();
			_color = c;
			_r = r;
			buildUI();
		}
		public function buildUI():void {
			graphics.clear();
			graphics.beginFill(_color);
			graphics.drawCircle(_r, _r, _r);
			graphics.endFill();
		}
		public function set color(c:uint):void {
			_color = c;
			buildUI();
		}
		public function get color():uint {
			return _color;
		}
		public function set r(r:Number):void {
			_r = r;
			buildUI();
		}
		public function get r():Number {
			return _r;
		}
	}
}