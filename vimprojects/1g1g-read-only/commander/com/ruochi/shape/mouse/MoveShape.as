package com.ruochi.shape.mouse {
	import flash.display.Shape;
	import flash.filters.DropShadowFilter;
	import flash.display.CapsStyle;
	import flash.display.JointStyle;
	public class MoveShape extends Shape{
		private var size:int = 4;
		public function MoveShape() {
			draw();
			//filters = [new DropShadowFilter(0, 0, 0, 1, 1.1, 1.1, 3,3)];
		}
		
		private function draw():void {
			//graphics.lineStyle(1, 0, 1, false, "normal", CapsStyle.SQUARE, JointStyle.BEVEL);
			graphics.lineStyle(1, 0, 1, false, "normal");
			//graphics.lineStyle(1, 0, 1);
			graphics.beginFill(0xffffff);
			
			graphics.moveTo(size * 3, size * 0);
			graphics.lineTo(size * 2, size * 1);
			graphics.lineTo(size * 3, size * 2);
			graphics.lineTo(size * 4, size * 1);
			graphics.lineTo(size * 3, size * 0);
			
			graphics.moveTo(size * 0, size * 3);
			graphics.lineTo(size * 1, size * 2);
			graphics.lineTo(size * 2, size * 3);
			graphics.lineTo(size * 1, size * 4);
			graphics.lineTo(size * 0, size * 3);
			
			graphics.moveTo(size * 2, size * 5);
			graphics.lineTo(size * 3, size * 6);
			graphics.lineTo(size * 4, size * 5);
			graphics.lineTo(size * 3, size * 4);
			graphics.lineTo(size * 2, size * 5);
			
			graphics.moveTo(size * 5, size * 2);
			graphics.moveTo(size * 4, size * 3);
			graphics.lineTo(size * 5, size * 4);
			graphics.lineTo(size * 6, size * 3);
			graphics.lineTo(size * 5, size * 2);
			
			
			/*graphics.drawRect(size * 9, size * 0, size * 1, size * 1);
			graphics.drawRect(size * 8, size * 1, size * 3, size * 1);
			graphics.drawRect(size * 7, size * 2, size * 5, size * 1);
			
			
			graphics.drawRect(size * 0, size * 7, size * 5, size * 5);
			//graphics.drawRect(size * 3, size * 3, size * 2, size * 2);
			graphics.drawRect(size * 14, size * 7, size * 5, size * 5);
			graphics.drawRect(size * 7, size * 14, size * 5, size * 5);
			*/
			
			/*graphics.drawCircle(size * 5, size * 1, size * 1.5);
			graphics.drawCircle(size * 5, size * 5, size * 1.5);
			graphics.drawCircle(size * 5, size * 9, size * 1.5);*/
			
			graphics.endFill();
			
		
		}
		
	}
	
}