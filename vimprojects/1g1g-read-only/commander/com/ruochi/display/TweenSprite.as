package com.ruochi.display {
	import com.ruochi.events.Eventer;
	import flash.display.Sprite;
	import flash.events.Event;
	public class TweenSprite extends Sprite {
		private var _px:Number;
		private var _py:Number;
		private var _moveSpeed:Number;
		private var _alpha:Number;
		private var _alphaSpeed:Number;
		private var _isVisible:Boolean;
		private static var _min:Number = .01;
		public function moveTo(px:Number, py:Number, moveSpeed:Number = 8) {
			_px = px;
			_py = py;
			_moveSpeed = moveSpeed;
			removeEventListener(Event.ENTER_FRAME, onMoveToEnterFrame);
			addEventListener(Event.ENTER_FRAME, onMoveToEnterFrame, false, 0, true);
		}
		
		private function onMoveToEnterFrame(e:Event):void {
			x = (_px - x) / _moveSpeed;
			y = (_py - y) / _moveSpeed;
			if (_px - x < _min && _py - y < _min) {
				x = _px;
				y = _py;
				removeEventListener(Event.ENTER_FRAME, onMoveToEnterFrame);
				dispatchEvent(new Eventer(Eventer.COMPLETE));
			}			
		}
		public function alphaTo(a:Number, alphaSpeed, isVisible:Boolean) {
			_alpha = a;
			_alphaSpeed = alphaSpeed;
			_isVisible = isVisible;
		}
	}	
}
	