package com.ruochi.shape{
	import flash.display.Shape;
	public class BottomRightTriangle extends Shape {
		private var _color:uint = 0xffffff;
		private var _w:Number;
		private var _h:Number;
		public function BottomRightTriangle(w:Number=100,h:Number=100,c:uint=0xffffff) {
			super();
			_color = c;
			_w = w;
			_h = h;
			draw()
		}
		private function draw():void {
			graphics.clear();
			graphics.beginFill(_color);
			graphics.moveTo(_w, 0);
			graphics.lineTo(_w, _h);
			graphics.lineTo(0, _h);
			graphics.lineTo(_w, 0);
			graphics.endFill();
		}
		
		public function set color(c:uint):void {
			_color = c;
			draw()
		}
		
		public function get color():uint {
			return _color;
		}
		
		override public function set width(value:Number):void {
			_w = value
			draw();
		}
		
		override public function set height(value:Number):void {
			_h = value;
			draw();
		}
	}
}