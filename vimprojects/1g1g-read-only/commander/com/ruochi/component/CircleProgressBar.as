package com.ruochi.component{
	import flash.display.Sprite;
	import flash.display.Shape;
	import flash.events.Event;
	import com.ruochi.shape.Rect;
	import com.ruochi.shape.LoaderShape;
	import com.ruochi.layout.setCenter;
	import flash.utils.Timer;
	import flash.events.TimerEvent;
	public class CircleProgressBar extends Sprite {
		protected var _r1:Number;
		protected var _r2:Number;
		protected var _speed:Number = 5;
		protected var _loaderColor:uint = 0xffffff;
		protected var _circle:LoaderShape = new LoaderShape(_r1, _r2, _loaderColor);
		protected var _isStart:Boolean = false;		
		private var _timer:Timer = new Timer(300, 1);
		public function CircleProgressBar(r1:Number = 10, r2:Number = 16) {
			_r1 = r1;
			_r2 = r2;
			visible = false;
			buildUI();
			_timer.addEventListener(TimerEvent.TIMER, onTimer, false, 0, true);
		}
		
		private function onTimer(e:TimerEvent):void {
			if(_isStart){
				visible = true;
			}
		}
		protected function buildUI():void {
			_circle = new LoaderShape(_r1, _r2, _loaderColor);
			addChild(_circle);		
		}
		protected function onEnterFrame(e:Event):void{
			_circle.rotation += _speed;
		}
		public function start():void {
			if (!_isStart) {
				_timer.start();
				addEventListener(Event.ENTER_FRAME, onEnterFrame, false, 0, true);
				_isStart = true;
			}		
		}
		public function stop():void {
			_timer.stop();
			_timer.reset();
			visible = false;
			removeEventListener(Event.ENTER_FRAME, onEnterFrame);
			_isStart = false;
		}
		public function set loaderColor(col:Number):void {
			_loaderColor = col;
			_circle.color = _loaderColor;
		}
		public function get circle():LoaderShape{
			return _circle;
		}
	}
}