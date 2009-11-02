package com.ruochi.shape{
	import flash.display.Shape;
	import com.ruochi.graphic.drawArc
	public class BottomCornerShape extends Shape {
		private var _width:Number;
		private var _height:Number;
		private var _r:Number;
		private var _color:uint;
		public function BottomCornerShape(w:Number=200,h:Number=200,r:Number=25,c:uint=0xff0000) {
			super()
			_width = w;
			_height = h;
			_r = r;
			_color = c;
			draw()
		}
		private function draw():void  {
			graphics.clear();
			graphics.beginFill(_color);
			graphics.moveTo(0, 0);
			graphics.lineTo(_width , 0);
			graphics.lineTo(_width, _height - _r);
			drawArc(this, _width - _r, -_height +_r, _r, -90, 0);
			graphics.lineTo(_r, _height);
			drawArc(this, _r, -_height +_r, _r, -90, 270);
			graphics.lineTo(0, 0);
            graphics.endFill();
		}
		public function set color(c:uint):void  {
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