package com.ruochi.shape.arrow {
	import flash.display.Shape;
	public class ArrowRightShape extends Shape {
		private var _color:uint;
		private var _size:int;
		public function ArrowRightShape(color:uint = 0xffffff, size:int = 4) {
			_color = color;
			_size = size;
			draw()
		}
		private function draw():void {
			graphics.clear();
			graphics.beginFill(_color);
			for (var i:int = 0; i < _size; i++) {
				graphics.drawRect(i, i, 1, (_size - i) * 2);
			}
			graphics.endFill();			
		}
		public function set color(c:uint):void {
			_color = c;
			draw();
		}
	}
}