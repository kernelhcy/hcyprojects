package com.ruochi.shape {
import flash.display.Shape;
	public class MinusShape extends Shape {
		private var _color:uint;
		public function MinusShape(color:uint = 0xffffff) {
			_color = color;
			draw()
		}
		private function draw():void {
			graphics.beginFill(_color);
			graphics.drawRect(0, 0, 9, 3);
			graphics.endFill();			
		}
	}
}