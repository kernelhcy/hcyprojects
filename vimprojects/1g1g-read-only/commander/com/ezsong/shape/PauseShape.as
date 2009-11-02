package com.ezsong.shape{
	import flash.display.Shape;
	import com.ruochi.graphic.drawArc;
	public class PauseShape extends Shape {
		private var _color:uint = 0xffffff;
		private var _size:int;
		private var _roundCornner:Number;
		public function PauseShape(size:int = 40, roundCornnet:Number = 3, c:uint = 0xffffff) {
			_color = c;
			_size = size;
			_roundCornner = roundCornnet;
			draw();
		}
		private function draw():void {
			graphics.clear();
			graphics.beginFill(_color);
			graphics.drawRoundRect(0, 0, int(_size/4), _size, _roundCornner);
			graphics.drawRoundRect(int(_size*.4), 0, int(_size/4), _size, _roundCornner);
			graphics.endFill();
		}
		public function set color(c:uint):void {
			_color = c;
			draw()
		}
		public function get color():uint {
			return _color;
		}
	}
}