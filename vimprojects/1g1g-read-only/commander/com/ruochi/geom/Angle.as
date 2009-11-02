package com.ruochi.geom {
	import flash.geom.Point;
	public class Angle {
		private var _radian:Number;
		public static function PoiotToAngel(point1:Point, point2:Point = null):Angle {
			if (!point2) {
				point2 = new Point(0, 0);
			}
			var dy:Number = point1.y - point2.y;
			var dx:Number = point1.x - point2.x;
			return new Angle(Math.atan(dy / dx));
		}
		public function Angle(arc:Number = 0) {
			_radian = arc;
		}
		public function set degree(value:Number):void {
			_radian = value / 180 * Math.PI;
		}
		public function get degree():Number {
			return _radian / Math.PI * 180;
		}
		public function get radian():Number {
			return _radian;
		}
		public function set radian(value:Number):void {
			_radian = value;
		}
	}
}