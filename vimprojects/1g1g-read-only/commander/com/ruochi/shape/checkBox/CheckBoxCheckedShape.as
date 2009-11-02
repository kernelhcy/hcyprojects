package com.ruochi.shape.checkBox{
	import flash.display.Shape;
	public class CheckBoxCheckedShape extends Shape {
		private var _color:uint;
		public function CheckBoxCheckedShape(c:uint=0xffffff) {
			_color = c;
			draw();
		}
		private function draw():void {
			graphics.clear();
			graphics.beginFill(_color,.7);
			graphics.drawRect(0, 0, 1, 1);
			graphics.drawRect(0, 10, 1, 1);
			graphics.drawRect(10, 10, 1, 1);
			graphics.drawRect(10, 0, 1, 1);
			graphics.endFill();
			graphics.beginFill(_color);
			graphics.drawRect(1, 0, 9, 1);
			graphics.drawRect(0, 1, 1, 9);
			graphics.drawRect(10, 1, 1, 9);
			graphics.drawRect(1, 10, 9, 1);
			graphics.drawRect(2, 4, 1, 3);
			graphics.drawRect(3, 5, 1, 3);
			graphics.drawRect(4, 6, 1, 3);
			graphics.drawRect(5, 5, 1, 3);
			graphics.drawRect(6, 4, 1, 3);
			graphics.drawRect(7, 3, 1, 3);
			graphics.drawRect(8, 2, 1, 3);			
            graphics.endFill();
		}
		public function set color(c:uint):void {
			_color = c;
			draw();
		}
		public function get color():uint {
			return _color;
		}
	}
}