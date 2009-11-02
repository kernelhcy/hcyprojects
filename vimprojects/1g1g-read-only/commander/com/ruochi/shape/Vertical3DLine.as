package com.ruochi.shape{
	import flash.display.Shape;
	public class Vertical3DLine extends Shape {
		private var _height:Number;
		public function Vertical3DLine(h:Number=100) {
			_height = h;
			draw()
		}
		private function draw():void {
			graphics.clear();
			graphics.beginFill(0x000000);
			graphics.drawRect(0, 0, 1, _height);
			graphics.beginFill(0xffffff,.5);
			graphics.drawRect(1, 0, 1, _height);
			graphics.endFill();
		}
	}
}