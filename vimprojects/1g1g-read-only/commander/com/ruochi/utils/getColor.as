package com.ruochi.utils{	
	public function getColor(ang:Number):uint {
		var r:uint;
		var g:uint;
		var b:uint;
		if (ang > 0 && ang <= 60) {
			r = 255;
			g = Math.round((ang / 60) * 255);
			b = 0;
		} else if (ang > 60 && ang <= 120) {
			r = Math.round(((120 - ang) / 60) * 255);
			g = 255;
			b = 0;
		} else if (ang > 120 && ang <= 180) {
			r = 0;
			g = 255;
			b = Math.round(((ang - 120) / 60) * 255);
		} else if (ang > 180 && ang <= 240) {
			r = 0;
			g = Math.round(((240 -ang) / 60) * 255);
			b = 255;
		} else if (ang > 240 && ang <= 300) {
			r = Math.round(((ang -240) / 60) * 255);;
			g = 0;
			b = 255;
		} else if (ang > 300 && ang <= 360) {
			r = 255;
			g = 0;
			b = Math.round(((360 -ang) / 60) * 255);;
		}
		return r * 65536 + g * 256 + b;
	}
}