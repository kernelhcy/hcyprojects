package com.ruochi.display {
	import flash.display.Sprite;
	public class ElementContainer extends Sprite {
		protected var _width:int;
		protected var _height:int;
		public function ElementContainer() {
			
		}
		override public function get width():Number { return _width; }
		
		override public function set width(value:Number):void {
			_width = value;
		}
		
		override public function get height():Number { return _height; }
		
		override public function set height(value:Number):void {
			_height = value;
		}
		
		public function get actualWidth():Number {
			return super.width;
		}
		public function get actualHeight():Number {
			return super.height;
		}
	}	
}