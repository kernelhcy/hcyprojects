package com.ruochi.geom{
	public class Color {
		private var _color:uint;
		private var _r:uint;
		private var _g:uint;
		private var _b:uint;
		public function Color(c:uint=0) {
			color = c;
		}
		public function get color():uint{
			return _color
		}
		public function get red():uint{
			return _r
		}
		public function get green():uint{
			return _g
		}
		public function get blue():uint{
			return _b
		}
		public function set red(r:uint):void {
			_r = r;
			_color = sumRGB(_r, _g, _b);
		}
		public function set green(g:uint):void {
			_g = g;
			_color = sumRGB(_r, _g, _b);
		}
		public function set blue(b:uint):void {
			_b = b;
			_color = sumRGB(_r, _g, _b);
		}
		
		
		
		public function set color(value:uint):void{
			_color = value;
			_b = value & 0xff;
			value = value >> 8;
			_g = value & 0xff;
			value = value >> 8;
			_r = value & 0xff;
		}
		
		static public function sumRGB(r:uint, g:uint, b:uint):uint {
			return ((((r) << 8) | g) <<8) | b
		}
		
		static public function sum(colorA:uint, colorB:uint):uint {
			var color1:Color = new Color(colorA);
			var color2:Color = new Color(colorB);
			var r:uint = color1.red + color2.red;
			var g:uint =  color1.green + color2.green;
			var b:uint =  color1.blue + color2.blue; 
			return sumRGB(r, g, b);
		}
		
		static public function multiplication(value:uint, n:Number):uint {
			var c:Color = new Color(value);
			var r:uint = (c.red * n);
			var g:uint = (c.green * n);
			var b:uint = (c.blue * n);
			return sumRGB(r, g, b);
		}
	}
}