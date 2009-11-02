package com.ezsong.shape{
	import flash.display.Shape;
	import com.ruochi.graphic.drawArc;
	public class PlayShape extends Shape {
		private var _color:uint = 0xffffff;
		private var _conner:Number;
		private var _sideLength:Number;
		public function PlayShape(sideLength:Number =34, conner:Number = 3,c:uint=0xffffff) {
			_color = c;
			_sideLength = sideLength;
			_conner = conner;
			draw();
		}
		private function draw():void {
			graphics.clear();
			graphics.beginFill(_color);
			graphics.moveTo(0, _conner);
			graphics.lineTo(0, _conner + _sideLength);
			drawArc(this, _conner, -_conner - _sideLength, _conner, 120, -180);
			graphics.lineTo(_conner + _sideLength * Math.cos(Math.PI / 6)+ Math.sin(Math.PI / 6) * _conner, _conner + _sideLength/2 + _conner * Math.cos(Math.PI / 6));
			drawArc(this, _conner + _sideLength * Math.cos(Math.PI / 6), - _conner - _sideLength/2 , _conner, 120, -60);
			graphics.lineTo(_conner + _conner * Math.sin(Math.PI / 6), _conner - _conner*Math.cos(Math.PI / 6));
			drawArc(this, _conner, -_conner, _conner, 120, 60);
			graphics.endFill();
		}
		public function set color(c:uint):void {
			_color = c;
			draw()
		}
		public function get color():uint {
			return _color;
		}
		
		public function get sideLength():Number { return _sideLength; }
		
		public function set sideLength(value:Number):void {
			_sideLength = value;
			draw();
		}
		
		public function get conner():Number { return _conner; }
		
		public function set conner(value:Number):void {
			_conner = value;
			draw();
		}
	}
}