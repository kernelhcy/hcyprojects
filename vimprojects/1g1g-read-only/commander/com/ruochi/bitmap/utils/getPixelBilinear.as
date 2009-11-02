package com.ruochi.bitmap{
	import flash.display.Bitmap;
	import flash.display.BitmapData;
	public function getPixelBilinear(_bitmapData:BitmapData, theX:Number, theY:Number): Number {
			
			var x:int;
			var y:int;
			var x_ratio:Number;
			var y_ratio:Number;
			var y_opposite:Number;
			var x_opposite:Number;
			var a:int;
			var be:int;
			var c:int;
			var d:int;
			var red:int;
			var green:int;
			var blue:int;
			
			
			x = Math.floor(theX);
			y = Math.floor(theY);
			
			if((x < 1) || (y < 1) || ((x + 2) >= width) || ((y + 2) >= height))
				return _bitmapData.getPixel(x, y);
			
			x_ratio = theX - x;
			y_ratio = theY - y;
			x_opposite = 1 - x_ratio;
			y_opposite = 1 - y_ratio;
						
			a = _bitmapData.getPixel(x, y);
			be =_bitmapData.getPixel(x + 1, y);
			c = _bitmapData.getPixel(x, y + 1);
			d = _bitmapData.getPixel(x + 1, y + 1);
			red 	= (r(a)  * x_opposite  + r(be)   * x_ratio) * y_opposite + (r(c) * x_opposite  + r(d) * x_ratio) * y_ratio;
			green 	= (g(a)  * x_opposite  + g(be)   * x_ratio) * y_opposite + (g(c) * x_opposite  + g(d) * x_ratio) * y_ratio;
			blue 	= (b(a)  * x_opposite  + b(be)   * x_ratio) * y_opposite + (b(c) * x_opposite  + b(d) * x_ratio) * y_ratio;
				/*red = r(a);
				green = g(a);
				blue = b(a);*/
				
			if(red < 0)
				red = 0;
			else if(red > 255)
				red = 255;
			if(green < 0)
				green = 0;
			else if(green > 255)
				green = 255;
			if(blue < 0)
				blue = 0;
			else if(blue > 255)
				blue = 255;

			return (red << 16) | (green << 8) | (blue << 0);
		}
	}	
}