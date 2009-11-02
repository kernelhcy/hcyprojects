package com.ruochi.component {
	import flash.display.Sprite;
	import flash.events.Event;
	import com.ruochi.shape.StripeShape;
	import flash.geom.Rectangle;

	public class BufferingBar extends Sprite{		
		private var _stripeShape:StripeShape;
		private var _width:int;
		private var _height:int;
		private var _isEnterFrame:Boolean;
		public function BufferingBar(w:int,h:int) {
			_width = w;
			_height = h;
			init();
		}
		
		private function init():void{
			_stripeShape =  new StripeShape(_width + _height * 3, _height);
			//scrollRect = new Rectangle(0, 0, _width, _height);
			addChild(_stripeShape);
		}
		
		public function start():void {
			if (!_isEnterFrame) {
				addEventListener(Event.ENTER_FRAME, onEnterFrame, false, 0, true);
				_isEnterFrame = true;
			}
		}
		
		private function onEnterFrame(e:Event):void {
			_stripeShape.x++;
			if (_stripeShape.x >= 0) {
				_stripeShape.x = -2*_height;
			}
		}
		
		public function stop():void {
			removeEventListener(Event.ENTER_FRAME, onEnterFrame);
			_isEnterFrame = false;
		}
		override public function get width():Number { return _width; }
		
		override public function set width(value:Number):void {
			super.width = value;
		}
		
		public function get stripeShape():StripeShape { return _stripeShape; }
	}
	
}