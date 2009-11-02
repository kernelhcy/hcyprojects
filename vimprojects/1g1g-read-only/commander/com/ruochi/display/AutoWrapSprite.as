package com.ruochi.display {
	import flash.display.DisplayObject;
	import flash.display.Sprite;
	public class AutoWrapSprite extends Sprite {
		private var _width:int;
		private var _gapY:int=2;
		private var _gapX:int = 4;
		private var _marginTop:Number =0;
		private var _marginLeft:Number =0;
		private var _marginRight:Number =0;
		private var _marginBottom:Number =0;
		override public function get width():Number { return _width; }
		
		override public function set width(value:Number):void {
			_width = value;
			var i:int;
			var length:int = numChildren;
			var nextY:int = _marginTop;
			var nextX:int = _marginLeft;
			var displayObject:DisplayObject;
			for (i = 0; i < length; i++) {
				displayObject =  getChildAt(i);
				if (nextX + _gapX + displayObject.width > _width && i>0) {
					nextX = _marginLeft;
					nextY += displayObject.height + _gapY;
				}
				displayObject.x = nextX;
				displayObject.y = nextY;
				nextX += displayObject.width + _gapX;
			}
		}
		override public function get height():Number { return super.height+ _marginBottom + _marginTop; }
		
		override public function set height(value:Number):void {
			super.height = value;
		}
		
		public function get gapX():int { return _gapX; }
		
		public function set gapX(value:int):void {
			_gapX = value;
		}
		
		public function get marginTop():Number { return _marginTop; }
		
		public function set marginTop(value:Number):void {
			_marginTop = value;
		}
		
		public function get marginBottom():Number { return _marginBottom; }
		
		public function set marginBottom(value:Number):void {
			_marginBottom = value;
		}
		
		public function get marginLeft():Number { return _marginLeft; }
		
		public function set marginLeft(value:Number):void {
			_marginLeft = value;
		}
	}	
}
	