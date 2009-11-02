package com.ruochi.events {
	import flash.events.Event;
	import flash.events.EventDispatcher;
	import flash.events.MouseEvent;
	import flash.display.InteractiveObject;
	import flash.utils.Timer;
	import flash.events.TimerEvent;
	public class ClickAndDoubleClick extends EventDispatcher {
		private var _interactiveObject:InteractiveObject;
		private var _timer:Timer;
		private var _interval:Number;
		public function ClickAndDoubleClick(interactiveObject:InteractiveObject, interval:Number=0.3) {
			_interactiveObject = interactiveObject;
			_timer = new Timer(interval * 1000, 1);
			_interval = interval;
			_interactiveObject.addEventListener(MouseEvent.CLICK, onInteractiveObjectClick, false, 0 , true);
			_timer.addEventListener(TimerEvent.TIMER, onTimer, false, 0, true);
		}
		
		private function onTimer(e:TimerEvent):void {
			dispatchEvent(new MouseEvent(MouseEvent.CLICK));
		}
		
		private function onInteractiveObjectClick(e:MouseEvent):void {
			if (_timer.running) {
				dispatchEvent(new MouseEvent(MouseEvent.DOUBLE_CLICK));
				_timer.reset();
			}else {
				_timer.start();
			}
		}
		
	}
}