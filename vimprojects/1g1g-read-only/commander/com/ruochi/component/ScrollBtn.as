package com.ruochi.component{
	import com.ruochi.events.Eventer;
	import flash.display.DisplayObject;
	import flash.display.Sprite;
	import flash.events.Event;
	import flash.events.EventDispatcher;
	import flash.geom.Rectangle;
	import flash.utils.Timer;
	import flash.events.TimerEvent;
	import flash.events.MouseEvent;
	import com.ruochi.utils.*;
	public class ScrollBtn extends EventDispatcher {
		
		private var _upBtn:Sprite;
		private var _downBtn:Sprite;
		private var _sourceClip:Sprite;
		private var _sourceMask:DisplayObject;
		private var _step:Number =24;
		private var _speed:Number = 5;
		private var _speedDefault:Number = 5;
		private var _G:Number = 0;
		private var _scrollOrientation:String;
		private var _delayTimer:Timer;
		private var _stepId:int;
		private var _isMoving:Boolean;
		public function ScrollBtn(src:Sprite, mask:DisplayObject,upB:Sprite,downB:Sprite) {
			_sourceClip=src;
			_sourceMask=mask;
			_stepId=0;
			_delayTimer=new Timer(50,0);
			_delayTimer.addEventListener(TimerEvent.TIMER,onTimer,false,0,true);
			_upBtn=upB;
			_downBtn = downB;
			init();
		}
		public function init():void {			
			_upBtn.addEventListener(MouseEvent.MOUSE_DOWN, onUpBtnDown, false, 0, true); 
			_upBtn.addEventListener(MouseEvent.MOUSE_UP, onUpBtnUp, false, 0, true);
			_downBtn.addEventListener(MouseEvent.MOUSE_DOWN,onDownBtnDown,false,0,true);
			_downBtn.addEventListener(MouseEvent.MOUSE_UP, onDownBtnUp, false, 0, true);
			_upBtn.addEventListener(Event.ADDED_TO_STAGE, onUpBtnAddToStage, false, 0, true);
			//_sourceClip.addEventListener(MouseEvent.MOUSE_WHEEL, onMouseWheel, false, 0, true);
		}
		
		private function onUpBtnAddToStage(e:Event):void {
			_upBtn.stage.addEventListener(MouseEvent.MOUSE_UP, onMouseUpOutside, false, 0, true);
		}
		
		public function scroll(delta:int):void {
			var i:int;
			if (delta > 0) {
				for (i=0; i < delta; i++) {
					if(upEnable){
						if ((Math.floor((_G/_step))+1)*_step<=_position2) {
							_G=(Math.floor((_G/_step))+1)*_step;
						}
					}
				}
			}else if(delta<0) {
				for (i=0; i < -delta; i++) {
					if(downEnable){
						if ((Math.floor((_G/_step))-1)*_step>=_position1) {
							_G=(Math.floor((_G/_step))-1)*_step;
						}
					}
				}
			}
			setEnterFrame();
		}
		private function onTimer(e:TimerEvent):void {
			if (_delayTimer.currentCount>5) {
				_speed=(_speed-1)*0.98+1;
				if (_scrollOrientation=="down") {
					if(downEnable){
						if ((Math.floor((_G/_step))-1)*_step>=_position1) {
							_G=(Math.floor((_G/_step))-1)*_step;
						}
						//_sourceClip.addEventListener(Event.ENTER_FRAME,EnterFrame,false,0,true);
					}
				} else {
					if(upEnable){
						if ((Math.floor((_G/_step))+1)*_step<=_position2) {
							_G=(Math.floor((_G/_step))+1)*_step;
						}
					}
				}				
			}
		}
		private function onUpBtnDown(e:MouseEvent):void {
			scrollUp();			
			_delayTimer.start();
		}
		private function scrollUp():void {
			_scrollOrientation="up";
			if (upEnable) {
				if (_sourceClip.height>_sourceMask.height) {
					if ((Math.floor(_sourceClip.y/_step)+1)*_step<=_position2) {
						_G=(Math.floor((_sourceClip.y/_step))+1)*_step;
					}
				}
				setEnterFrame()
			} 
		}
		private function setEnterFrame():void {
			_sourceClip.removeEventListener(Event.ENTER_FRAME, EnterFrame);
			_sourceClip.addEventListener(Event.ENTER_FRAME, EnterFrame);
			_isMoving = true;
			dispatchEvent(new Eventer(Eventer.START));
			
		}
		private function onDownBtnUp(e:MouseEvent):void {
			_delayTimer.reset();
			_speed = _speedDefault;
		}
		private function onDownBtnDown(e:MouseEvent):void {
			scrollDown();	
			_delayTimer.start();
		}
		private function scrollDown():void {
			_scrollOrientation="down";
			if (downEnable) {
				if ((Math.floor(_sourceClip.y/_step)-1)*_step>=_position1) {
					_G=(Math.floor((_sourceClip.y/_step))-1)*_step;
				}
				setEnterFrame()
			} 
		}
		private function onUpBtnUp(e:MouseEvent):void {
			_delayTimer.reset();
			_speed = _speedDefault;
		}
		private function EnterFrame(e:Event):void { trace('bbbbbbbbb');
			_sourceClip.y+=(_G-_sourceClip.y)/_speed;
			if (Math.abs(_sourceClip.y-_G)<1) {
				_sourceClip.y=_G;
				_stepId = Math.floor((_sourceMask.y - _sourceClip.y) / _step);
				_sourceClip.removeEventListener(Event.ENTER_FRAME, EnterFrame);
				dispatchEvent(new Eventer(Eventer.COMPLETE));
				checkEnable();
				_isMoving = false;
			}
		}
		public function checkEnable():void {
			
			if (_sourceClip.height + _sourceClip.y < _sourceMask.height + _sourceMask.y && _sourceClip.y < _sourceMask.y) {
				scrollBottom();
			}
			if (_sourceClip.y > _sourceMask.y) {
				scrollTop();
			}
			dispatchEvent(new Eventer(Eventer.CHECK_ENABLE));
		}
		private function get _position1():Number {
			return Math.floor((_sourceMask.y - _sourceClip.height + _sourceMask.height) / _step) * _step;
		}
		private function get _position2():Number {
			return Math.ceil(_sourceMask.y / _step) * _step;
		}
		public function scrollTop():void {
			_G = Math.floor((_position2 / _step)) * _step;
			setEnterFrame()
		}
		public function scrollBottom():void {
			_G = Math.floor((_position1 / _step)) * _step;
			setEnterFrame()
		}
		private function onMouseUpOutside(e:Event):void{
			_delayTimer.reset();
			_speed = _speedDefault;
		}
		public function set step(num:Number):void {
			_step = num;
		}
		public function get downEnable():Boolean {
			var _downEnable:Boolean;
			if ((Math.floor(_sourceClip.y/_step)-1)*_step<_position1) {
				_downEnable = false;
			} else {
				_downEnable = true;
			}			
			return _downEnable;
		}
		public function get upEnable():Boolean {
			var _upEnable:Boolean;
			if ((Math.floor(_sourceClip.y/_step)+1)*_step>_position2) {
				_upEnable = false;	
			} else {
				_upEnable = true;
			}
			return _upEnable;
		}
		
		public function get isMoving():Boolean { return _isMoving; }
	}
}