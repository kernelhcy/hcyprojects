package com.ruochi.shape{
	import flash.display.Shape;
	import flash.display.SpreadMethod;
	import flash.display.GradientType;
	import flash.geom.Matrix;
	import flash.geom.Rectangle;
	public class GradientRadialTopLeftRect extends Shape {
		private var _w:Number;
		private var _h:Number;
		private var _color1:uint;
		private var _color2:uint;
		private var _alpha1:Number;
		private var _alpha2:Number;
		public function GradientRadialTopLeftRect(w:Number=100,h:Number=100,c1:uint=0xffffff,c2:uint=0x000000,alpha1:Number = 100 ,alpha2:Number = 100) {
			super()
			_w = w;
			_h = h;
			_color1 = c1;
			_color2 = c2;
			_alpha1 = alpha1;
			_alpha2 = alpha2;
			draw();
		}
		private function draw():void {
			this.graphics.clear()
			var fillType:String = GradientType.RADIAL;
			var colors:Array = [_color1, _color2];
			var alphas:Array = [_alpha1, _alpha2]; 
			var ratios:Array = [0x00, 0xFF];
			var matr:Matrix = new Matrix();
			matr.createGradientBox(_w*2.5, _h*2.5, 0, -_w, -_h);
			var spreadMethod:String = SpreadMethod.PAD;
			this.graphics.beginGradientFill(fillType, colors, alphas, ratios, matr, spreadMethod);
            this.graphics.drawRect(0, 0, _w, _h);			
            this.graphics.endFill();
		}
		public function set color1(col:uint):void {
			_color1 = col;
			draw();
		}
		public function set color2(col:uint):void {
			_color2 = col;
			draw();
		}
	}
}