package com.ruochi.utils {
	import flash.utils.Timer;
	import flash.events.TimerEvent;
	public class DelayCall{
		private var _timer:Timer;
		private var _funciton:Function;
		private var _functionParams:Array;
		public function DelayCall(time:Number, callFunction:Function, callFunctionParams:Array = null) {
			_timer  = new Timer(time * 1000, 1);
			_funciton = callFunction;
			_functionParams = callFunctionParams;
			_timer.addEventListener(TimerEvent.TIMER, onTimer);
			_timer.start();
		}		
		private function onTimer(e:TimerEvent):void {
			_timer.removeEventListener(TimerEvent.TIMER, onTimer);
			_funciton.apply(this, _functionParams);
		}
		public static function call(time:Number, callFunction:Function, callFunctionParams:Array = null):DelayCall {
			return new DelayCall(time, callFunction, callFunctionParams);
		}
		public function stop():void {
			_timer.stop();
		}
	}
}