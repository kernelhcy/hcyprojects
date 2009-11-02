package com.ruochi.shape.arrow {
	import flash.display.Shape;
	public class ArrowDownShape extends Shape {
		private var _color:uint;
		private var _size:int;
		public function ArrowDownShape(color:uint = 0xffffff, size:int = 4){
			_color = color;
			_size = size;
			draw()
		}
		private function draw():void {
			graphics.clear();
			graphics.beginFill(_color);
			for (var i:int = 0; i < _size; i++) {
				graphics.drawRect(i, i, (_size - i) * 2, 1);
			}
			graphics.endFill();			
		}
		public function color(c:uint):void {
			_color = c;
			draw();
		}
	}
}