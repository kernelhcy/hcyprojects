package com.ruochi.shape{
	import flash.display.Shape;
	import com.ruochi.graphic.drawArc
	public class TabShape extends Shape {
		private var _width:Number;
		private var _height:Number;
		private var _r:Number;
		private var _color:uint;
		public function TabShape(w:Number=200,h:Number=200,r:Number=25,c:uint=0xff0000) {
			super()
			_width = w;
			_height = h;
			_r = r;
			_color = c;
			draw()
		}
		private function draw():void {
			graphics.clear();
			graphics.beginFill(_color);
			graphics.moveTo(_r, 0);
			graphics.lineTo(_width - _r , 0);
			drawArc(this, _width - _r, -_r, _r, -90, 90);
			graphics.lineTo(_width, _height);
			graphics.lineTo(0, _height);
			graphics.lineTo(0, _r);
			drawArc(this, _r , - _r, _r, -90, 180);
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
		override public function get width():Number { return super.width; }
		
		override public function set width(value:Number):void {
			_width = value;
			draw();			
		}
		override public function get height():Number { return super.height; }
		
		override public function set height(value:Number):void {
			_height = value;
			draw();
		}
	}
}