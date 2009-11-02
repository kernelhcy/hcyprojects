package com.ruochi.layout {
	public class Gap {
		public static const AUTO:String = "auto";
		private var _gapValue:Number;
		public function Gap(gapValue:Number = 0) {
			_gapValue = gapValue;
		}
		public function get isAuto():Boolean {
			return isNaN(_gapValue);
		}
		
		public function set isAuto(value:Boolean):void {
			if(value){
				_gapValue = Number.NaN;
			}
		}
		
		public function get value():Number {
			if (isAuto) {
				return 0
			}else {
				return _gapValue;
			}
		}
		public function set value(value:Number):void {
			_gapValue = value
		}
	}	
}