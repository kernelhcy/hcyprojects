package com.ruochi.shape{
	import flash.display.Shape;
	import com.ruochi.graphic.drawArc
	public class BottomCornerShape extends Shape {
		private var _w:Number;
		private var _h:Number;
		private var _r:Number;
		private var _color:uint;
		public function BottomCornerShape(w:Number=200,h:Number=200,r:Number=25,c:uint=0xff0000) {
			super()
			_w = w;
			_h = h;
			_r = r;
			_color = c;
			buildUI()
		}
		private function buildUI() {
			graphics.clear();
			graphics.beginFill(_color);
			graphics.moveTo(0, 0);
			graphics.lineTo(_w , 0);
			graphics.lineTo(_w, _h - _r);
			drawArc(this, _w - _r, -_h +_r, _r, -90, 0);
			graphics.lineTo(_r, _h);
			drawArc(this, _r, -_h +_r, _r, -90, 270);
			graphics.lineTo(0, 0);
            graphics.endFill();
		}
		public function set color(c:uint) {
			_color = c;
			buildUI();
		}
		public function get color():uint {
			return _color;
		}
		public function set corner(r:uint) {
			_r = r;
			buildUI();
		}
	}
}