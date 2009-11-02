package com.ruochi.shape {
	import flash.display.Shape;
	public class CloseShape extends Shape {
		private var _color:uint;
		private var _size:int;
		public function CloseShape(color:uint = 0xffffff) {
			_color = color;
			draw();
		}
		private function draw():void {
			graphics.clear();
			graphics.beginFill(_color);
			graphics.drawRect(0, 0, 2, 1);
			graphics.drawRect(5, 0, 2, 1);
			graphics.drawRect(1, 1, 2, 1);
			graphics.drawRect(4, 1, 2, 1);
			graphics.drawRect(2, 2, 3, 2);
			graphics.drawRect(4, 4, 2, 1);
			graphics.drawRect(1, 4, 2, 1);
			graphics.drawRect(5, 5, 2, 1);
			graphics.drawRect(0, 5, 2, 1);
			
			
			graphics.endFill();			
		}
		public function color(c:uint):void {
			_color = c;
			draw();
		}
	}
}