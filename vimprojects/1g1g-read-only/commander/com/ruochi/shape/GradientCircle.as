package com.ruochi.shape{
	import flash.display.Shape;
	import flash.geom.Matrix;
	import flash.display.GradientType;
	import flash.display.SpreadMethod;
	public class GradientCircle extends Shape {
		private var _r:Number;
		private var _color1:uint;
		private var _color2:uint;
		public function GradientCircle(r:Number=10,c1:uint=0x000000,c2:uint=0x000000) {
			super()
			_r = r;
			_color1 = c1;
			_color2 = c2;
			buildUI();
		}
		public function buildUI():void {
			this.graphics.clear()
			var fillType:String = GradientType.LINEAR;
			var colors:Array = [_color1, _color2];
			var alphas:Array = [0, 100];
			var ratios:Array = [0x00, 0xFF];
			var matr:Matrix = new Matrix();
			matr.createGradientBox(2*_r, 2*_r, Math.PI / 2, 0, 0);
			var spreadMethod:String = SpreadMethod.PAD;
			this.graphics.beginGradientFill(fillType, colors, alphas, ratios, matr, spreadMethod);
            this.graphics.drawCircle(_r, _r, _r);			
            this.graphics.endFill();
		}
		public function set corner(r:uint):void {
			_r = r;
			buildUI();
		}
		public function set color1(c:uint):void {
			_color1 = c;
			buildUI();
		}
		public function set color2(c:uint):void {
			_color2 = c;
			buildUI();
		}
	}
}