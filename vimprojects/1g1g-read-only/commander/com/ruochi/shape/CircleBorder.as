package com.ruochi.shape{
	import flash.display.Shape;
	public class CircleBorder extends Shape {
		public var _r:Number;
		public var _color:uint;
		public var _borderThick:Number;
		public function CircleBorder(r:Number = 100, bt:Number=1, c:uint = 0xffffff) {
			_color = c;
			_r = r;
			_borderThick = bt;
			super();
			buildUI()
		}
		public function buildUI():void {
			this.graphics.clear();
			this.graphics.beginFill(_color);
			this.graphics.drawCircle(_r, _r, _r);
			this.graphics.drawCircle(_r, _r, _r - _borderThick);
			this.graphics.endFill();
		}
		public function set color(c:uint):void {
			_color = c;
			buildUI();
		}
		public function get color():uint {
			return _color;
		}
		public function set borderThick(bt:Number):void {
			_borderThick = bt;
			buildUI();
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