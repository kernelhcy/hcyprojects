package com.ruochi.shape.mouse {
	import flash.display.Shape;
	public class CloseShape extends Shape{
		private var size:int = 3;
		public function CloseShape() {
			draw();
		}
		
		private function draw():void {
			graphics.lineStyle(1);
			graphics.beginFill(0xffffff);
			graphics.moveTo(size * 1, size * 0);
			graphics.lineTo(size * 0, size * 1);
			graphics.lineTo(size * 2, size * 3);
			graphics.lineTo(size * 0, size * 5);
			graphics.lineTo(size * 1, size * 6);
			graphics.lineTo(size * 3, size * 4);
			graphics.lineTo(size * 5, size * 6);
			graphics.lineTo(size * 6, size * 5);
			graphics.lineTo(size * 4, size * 3);
			graphics.lineTo(size * 6, size * 1);
			graphics.lineTo(size * 5, size * 0);
			graphics.lineTo(size * 3, size * 2);
			graphics.lineTo(size * 1, size * 0);
			graphics.endFill();
			
		
		}
		
	}
	
}