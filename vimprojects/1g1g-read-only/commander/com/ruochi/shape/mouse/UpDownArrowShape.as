package com.ruochi.shape.mouse {
	import flash.display.Shape;
	public class UpDownArrowShape extends Shape{
		private var size:int = 3;
		private var arrowDownY:int = 20;
		public function UpDownArrowShape() {
			draw();
		}
		
		private function draw():void {
			graphics.lineStyle(1, 0, 1);
			graphics.beginFill(0xffffff);
			
			/*
			graphics.moveTo(size * 2, size * 0);
			graphics.lineTo(size * 0, size * 2);
			graphics.lineTo(size * 4, size * 2);
			graphics.lineTo(size * 2, size * 0);
			
			graphics.moveTo(size * 0, size * 4);
			graphics.lineTo(size * 2, size * 6);
			graphics.lineTo(size * 4, size * 4);
			graphics.lineTo(size * 0, size * 4);
			*/
			
			/*graphics.moveTo(size * 2, size * 0);
			graphics.lineTo(size * 0, size * 2);
			graphics.lineTo(size * 0, size * 4);
			graphics.lineTo(size * 2, size * 2);
			graphics.lineTo(size * 4, size * 4);
			graphics.lineTo(size * 4, size * 2);
			graphics.lineTo(size * 2, size * 0);
			
			graphics.moveTo(size * 0, size * arrowDownY);
			graphics.lineTo(size * 0, size * (arrowDownY + 2));
			graphics.lineTo(size * 2, size * (arrowDownY + 4));
			graphics.lineTo(size * 4, size * (arrowDownY + 2));
			graphics.lineTo(size * 4, size * (arrowDownY + 0));
			graphics.lineTo(size * 2, size * (arrowDownY + 2));
			graphics.lineTo(size * 0, size * arrowDownY);			
			*/
			
			
			graphics.moveTo(size * 3, size * 0);
			graphics.lineTo(size * 0, size * 3);
			graphics.lineTo(size * 1, size * 4);
			graphics.lineTo(size * 3, size * 2);
			graphics.lineTo(size * 5, size * 4);
			graphics.lineTo(size * 6, size * 3);
			graphics.lineTo(size * 3, size * 0);
			
			
			graphics.moveTo(size * 1, size * 0 + arrowDownY);
			graphics.lineTo(size * 0, size * 1 + arrowDownY);
			graphics.lineTo(size * 3, size * 4 + arrowDownY);
			graphics.lineTo(size * 6, size * 1 + arrowDownY);
			graphics.lineTo(size * 5, size * 0 + arrowDownY);
			graphics.lineTo(size * 3, size * 2 + arrowDownY);
			graphics.lineTo(size * 1, size * 0 + arrowDownY);
			
			graphics.endFill();		
		}
		
	}
	
}