package com.ruochi.utils{
	public function getAngle(_x:Number, _y:Number):Number {
		if (_x > 0 && _y > 0) {
			return Math.atan(_y / _x) / Math.PI * 180;
		}else if (_x > 0 && _y < 0) {
			return Math.atan(_y / _x) / Math.PI * 180 + 360;
		}else {
			return Math.atan(_y / _x) / Math.PI * 180 + 180;
		}
	}
}