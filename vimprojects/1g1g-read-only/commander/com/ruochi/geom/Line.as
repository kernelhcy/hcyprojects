package com.ruochi.geom {
	import com.robertpenner.bezier.CubicCurve;
	import flash.geom.Point;
	import com.ruochi.geom.Angle;
	public class Line {
		private var _k:Number;
		private var _b:Number;
		public static function PoiotToAngel(point:Point):Angle {
			if (point.x > 0 && point.y > 0) {
				return new Angle(Math.atan(point.y / point.x) / Math.PI * 180);
			}else if (point.x > 0 && point.y < 0) {
				return new Angle(Math.atan(point.y / point.x) / Math.PI * 180 + 360);
			}else {
				return new Angle(Math.atan(point.y / point.x) / Math.PI * 180 + 180);
			}
		}
		public function Line(p1:Point=null,p2:Point=null) {
			if (p1 && p2) {
				_k =   (p1.y - p2.y) / (p1.x - p2.x);
				_b = p1.y - _k * p1.x;
			}
		}
		public function setPointAngle(p:Point, a:Angle):void {
			_k = Math.tan(a.radian);
			_b = p.y - _k * p.x;
		}
		public function getX(y:Number):Number {
			return (y - _b) / _k;
		}
		public function getY(x:Number):Number {
			return _k * x + _b;
		}
		public static function getIntersection(line1:Line, line2:Line):Point {
			var p:Point;
			if (line1.k != line2.k) {
				p = new Point();
				p.x = (line1.b - line2.b) / (line2.k - line1.k);
				p.y = line1.k * p.x + line1.b;
			}
			return p;
		}
		public function get b():Number {
			return _b;
		}
		public function get k():Number {
			return _k
		}
	}
}