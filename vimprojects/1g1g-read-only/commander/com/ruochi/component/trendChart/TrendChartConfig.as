package com.ruochi.component.trendChart {
	import com.ruochi.utils.stringToNum;
	public class TrendChartConfig {
		public static var xMax:Number;
		public static var xMin:Number;
		public static var yMax:Number;
		public static var yMin:Number;
		public static var chartWidth:Number;
		public static var chartHeight:Number;
		public static var marginTop:Number = 30;
		public static var marginRight:Number = 30;
		public static var marginBottom:Number = 30;
		public static var marginLeft:Number = 30;
		public static var isShowLabel:Boolean = true;
		public static function set xMaxValue(n:Object):void {
			xMax = stringToNum(String(n));
		}
		public static function set xMinValue(n:Object):void {
			xMin = stringToNum(String(n));
		}
		public static function set yMaxValue(n:Object):void {
			yMax = stringToNum(String(n));
		}
		public static function set yMinValue(n:Object):void {
			yMin = stringToNum(String(n));
		}
	}
}