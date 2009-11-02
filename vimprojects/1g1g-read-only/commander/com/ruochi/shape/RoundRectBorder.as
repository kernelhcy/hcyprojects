package com.ruochi.shape{
	import flash.display.Shape;
	public class RoundRectBorder extends Shape {
		private var _width:Number;
		private var _height:Number;
		private var _r:Number;
		private var _color:uint;
		private var _borderWidth:Number;
		public function RoundRectBorder(w:Number=100,h:Number=100,r:Number=10,borderWidth:Number=1,color:uint=0xff0000) {
			super()
			_width = w;
			_height = h;
			_r = r;
			_borderWidth = borderWidth;
			_color = color;
			draw();
		}
		private function draw():void {
			graphics.clear();
			graphics.beginFill(_color);
            graphics.drawRoundRect(0, 0, _width, _height, _r * 2);
			graphics.drawRoundRect(_borderWidth, _borderWidth, _width - _borderWidth * 2, _height - _borderWidth * 2, (_r - _borderWidth) * 2);
            graphics.endFill();
		}
		public function set color(c:uint):void {
			_color = c;
			draw();
		}
		public function set corner(c:Number):void {
			_r = c;
			draw();
		}
		public function set borderWidth(w:Number):void {
			_borderWidth = w;
			draw();
		}
		override public function set width(value:Number):void {
			_width = value;
			draw();
		}
		override public function set height(value:Number):void {
			_height = value;
			draw();
		}
	}
}