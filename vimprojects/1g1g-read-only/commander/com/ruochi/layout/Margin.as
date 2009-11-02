package com.ruochi.layout {
	public class Margin {
		private var _top:Gap;
		private var _bottom:Gap;
		private var _left:Gap;
		private var _right:Gap;
		public function Margin(margin:* = "0 0 0 0") {
				var array:Array = String(margin).split(" ");
				if(array.length ==4){
					top = new Gap(array[0]);
					right = new Gap(array[1]);
					bottom = new Gap(array[2]);
					left = new Gap(array[3]);
				}else if(array.length ==2){
					top = new Gap(array[0]);
					right = new Gap(array[1]);
					bottom = new Gap(array[0]);
					left = new Gap(array[1]);
				}else if (array.length == 1) {
					top = new Gap(array[0]);
					right = new Gap(array[0]);
					bottom = new Gap(array[0]);
					left = new Gap(array[0]);
				}
		}
		
		public function get top():Gap { return _top; }
		
		public function set top(value:Gap):void {
			_top = value;
		}
		
		public function get bottom():Gap { return _bottom; }
		
		public function set bottom(value:Gap):void {
			_bottom = value;
		}
		
		public function get left():Gap { return _left; }
		
		public function set left(value:Gap):void {
			_left = value;
		}
		
		public function get right():Gap { return _right; }
		
		public function set right(value:Gap):void {
			_right = value;
		}
	}	
}