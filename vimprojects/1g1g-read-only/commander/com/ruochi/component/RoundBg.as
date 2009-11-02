package com.ruochi.component{
	import flash.display.Sprite;
	import com.ruochi.shape.RoundRectBorder;
	import com.ruochi.shape.GradientRoundRect;
	import com.ruochi.shape.RoundRect;
	import com.ruochi.utils.deleteAll;
	public class RoundBg extends Sprite {
		private var _width:Number;
		private var _height:Number;
		private var _r:Number;
		private var _color1:uint;
		private var _color2:uint;
		private var _borderWidth:Number;
		private var _gradient:GradientRoundRect;
		private var _border:RoundRectBorder;
		private var _borderColor:uint;
		public function RoundBg(w:Number=100,h:Number=100,r:Number=10,borderWidth:Number=1,color1:uint=0xff0000,color2:uint=0x990000) {
			super()
			_width = w;
			_height = h;
			_r = r;
			_borderWidth = borderWidth;
			_color1 = color1;
			_color2 = color2;
			buildUI();
		}
		private function buildUI():void  {
			deleteAll(this);
			_gradient = new GradientRoundRect(_width, _height, _r, _color1, _color2);
			_border = new RoundRectBorder(_width, _height, _r, _borderWidth, _borderColor);
			addChild(_gradient);
			addChild(_border);
		}
		public function set color1(col:uint):void {
			_color1 = col;
			_gradient.color1 = _color1;
		}
		public function set color2(col:uint):void {
			_color2 = col;
			_gradient.color2 = _color2;
		}
		public function set borderColor(col:uint):void {
			_borderColor = col;
			_border.color = _borderColor;
		}
		override public function set width(value:Number):void {
			_width = value;
			_gradient.width = _width;
			_border.width = _width;
		}
		override public function set height(value:Number):void {
			_height = value;
			_gradient.height = _height;
			_border.height = _height;
		}
	}
}