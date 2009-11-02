package com.ruochi.geom{
	public class To2D  {
		public static var fl:Number = 100;
		public static var centerX:Number=0;
		public static var centerY:Number=0;
		public static function getScale(_z:Number):Number {
			if (_z < 0) {
				return fl / (fl - _z);
			}else {
				return (fl + _z) / fl;
			}			
		}
		public static function getX(_x:Number, _z:Number):Number {
			return _x * getScale(_z) + centerX;
		}
		public static function getY(_y:Number, _z:Number):Number {
			return _y * getScale(_z) + centerY;
		}
	}
}