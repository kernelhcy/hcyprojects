package com.ruochi.shape{
	import flash.display.Shape;
	import flash.display.SpreadMethod;
	import flash.display.GradientType;
	import flash.geom.Matrix;
	import flash.geom.Rectangle;
	public class GradientRadialRect extends Shape {
		private var _w:Number;
		private var _h:Number;
		private var _color1:uint;
		private var _color2:uint;
		public function GradientRadialRect(w:Number=100,h:Number=100,c1:uint=0xffffff,c2:uint=0x000000) {
			super()
			_w = w;
			_h = h;
			_color1 = c1;
			_color2 = c2;
			buildUI()
		}
		private function buildUI():void {
			this.graphics.clear()
			var fillType:String = GradientType.RADIAL;
			var colors:Array = [_color1, _color2];
			var alphas:Array = [100, 100];
			var ratios:Array = [0x00, 0xFF];
			var matr:Matrix = new Matrix();
			matr.createGradientBox(_w*2, _h*2, 0, -_w/2,0);
			var spreadMethod:String = SpreadMethod.PAD;
			graphics.beginGradientFill(fillType, colors, alphas, ratios, matr, spreadMethod);
            graphics.drawRect(0, 0, _w, _h);			
            graphics.endFill();
		}
		public function set color1(col:uint):void {
			_color1 = col;
			buildUI();
		}
		public function set color2(col:uint):void {
			_color2 = col;
			buildUI();
		}
	}
}