package com.ezsong.shape{
	import flash.display.Shape;
	import flash.geom.Rectangle;
	public class StripeShape extends Shape {
		private var _w:Number;
		private var _h:Number;
		private var _color:uint;
		private var _gap:int = 3;
		public function StripeShape(w:Number = 80, h:Number = 10, c:uint = 0xffffff) {
			super()
			_w = w;
			_h = h;
			_color = c;
			draw();
		}
		private function draw():void { 
			graphics.clear();
			var num:int =( _w + _h)/_gap /2 +1;
			graphics.beginFill(_color);	
			for (var i:int = 0; i < num; i++) {
				graphics.moveTo(i * 2 *_gap, _h);
				graphics.lineTo((i * 2 + 1) * _gap, _h);
				graphics.lineTo((i * 2 + 1) * _gap - 24 , 0);
				graphics.lineTo(i * 2 * _gap - 24, 0);
				graphics.lineTo(i * 2 * _gap, _h);
			}
			graphics.endFill();
			scrollRect = new Rectangle(0, 0, _w, _h);
		}
		public function set color(c:uint):void {
			_color = c;
			draw();
		}
		public function get color():uint {
			return _color;
		}
		
		override public function get width():Number { return _w }
		
		override public function set width(value:Number):void {
			_w = value
			draw();
		}
		override public function get height():Number { return _h }
		
		override public function set height(value:Number):void {
			_h = value;
			draw();
		}
	}
}