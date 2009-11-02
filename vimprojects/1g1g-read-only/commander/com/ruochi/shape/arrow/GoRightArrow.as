package com.ruochi.shape.arrow {
	import flash.display.Shape;
	public class GoRightArrow extends Shape {
		private var _color:uint;
		private var _size:int;
		public function GoRightArrow(color:uint = 0xffffff, size:int=3) {
			_color = color;
			_size = size;
			draw();
		}
		private function draw():void {
			graphics.clear();
			graphics.beginFill(_color);
			/*graphics.lineTo(1 * _size, 1 * _size);
			graphics.lineTo(0 * _size, 2 * _size);
			graphics.lineTo(1 * _size, 2 * _size);
			graphics.lineTo(2* _size, 1 * _size);
			graphics.lineTo(1 * _size, 0 * _size);
			graphics.lineTo(0 * _size, 0 * _size);
			var gap:int = _size / 2;
			graphics.moveTo(1 * _size + gap, 0 * _size);
			graphics.lineTo(2 * _size + gap, 1 * _size);
			graphics.lineTo(1 * _size + gap, 2 * _size);
			graphics.lineTo(2 * _size + gap, 2 * _size);
			graphics.lineTo(3* _size + gap, 1 * _size);
			graphics.lineTo(2 * _size + gap, 0 * _size);
			graphics.lineTo(1 * _size + gap, 0 * _size);*/
			graphics.drawRect(0, 0, _size, 1);
			graphics.drawRect(1, 1, _size, 1);
			graphics.drawRect(2, 2, _size, 1);
			graphics.drawRect(3, 3, _size, 1);
			graphics.drawRect(4, 4, _size, 1);
			graphics.drawRect(3, 5, _size, 1);
			graphics.drawRect(2, 6, _size, 1);
			graphics.drawRect(1, 7, _size, 1);
			graphics.drawRect(0, 8, _size, 1);
			
			graphics.drawRect(6, 0, _size, 1);
			graphics.drawRect(7, 1, _size, 1);
			graphics.drawRect(8, 2, _size, 1);
			graphics.drawRect(9, 3, _size, 1);
			graphics.drawRect(10, 4, _size, 1);
			graphics.drawRect(9, 5, _size, 1);
			graphics.drawRect(8, 6, _size, 1);
			graphics.drawRect(7, 7, _size, 1);
			graphics.drawRect(6, 8, _size, 1);
			graphics.endFill();
		}
		public function set color(c:uint):void {
			_color = c;
			draw();
		}
	}
}