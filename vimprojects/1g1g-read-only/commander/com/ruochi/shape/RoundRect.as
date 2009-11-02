package com.ruochi.shape{
	import flash.display.Shape;
	import flash.geom.Rectangle;
	public class RoundRect extends Shape {
		private var _width:Number;
		private var _height:Number;
		private var _r:Number;
		private var _color:uint;
		public function RoundRect(w:Number=100,h:Number=100,r:Number=10,c:uint=0xff0000) {
			super()
			_width = w;
			_height = h;
			_r = r;
			_color = c;
			draw();
		}
		public function draw():void {
			graphics.clear();
			graphics.beginFill(_color);
			if(_width>0&&_height>0){
				graphics.drawRoundRect(0, 0, _width, _height, _r * 2 , _r * 2);
			}
            graphics.endFill();
		}
		public function set color(c:uint):void {
			_color = c;
			draw();
		}
		public function get color():uint {
			return _color;
		}
		public function set corner(r:uint):void {
			_r = r;
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