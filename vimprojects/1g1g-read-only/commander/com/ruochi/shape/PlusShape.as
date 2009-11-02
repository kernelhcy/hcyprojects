package com.ruochi.shape {
import flash.display.Shape;
	public class PlusShape extends Shape {
		private var _color:uint;
		public function PlusShape(color:uint = 0xffffff) {
			_color = color;
			draw()
		}
		private function draw():void {
			graphics.beginFill(_color);
			graphics.drawRect(0, 3, 9, 3);
			graphics.drawRect(3, 0, 3, 3);
			graphics.drawRect(3, 6, 3, 3);
			graphics.endFill();			
		}
	}
}