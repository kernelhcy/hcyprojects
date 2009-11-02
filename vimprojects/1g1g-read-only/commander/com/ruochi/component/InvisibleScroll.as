package com.ruochi.component{
	import com.ezsong.player.common.Logger;
	import com.ruochi.shape.mouse.UpDownArrowShape;
	import com.ruochi.shape.Rect;
	import flash.display.DisplayObject;
	import flash.display.Shape;
	import flash.display.Sprite;
	import flash.events.Event;
	import flash.events.EventDispatcher;
	import flash.events.MouseEvent;
	import com.ruochi.utils.MouseUtils;
	import com.ruochi.utils.DelayCall;
	import com.ruochi.common.CommonText;
	import flash.utils.getTimer;
	public class InvisibleScroll extends Sprite {
		private var _content:DisplayObject;
		private var _mask:Shape;
		private var _direction:String;
		private var _maxSpeed:Number;
		private var _easeSpeed:Number = 3;
		private var _isEnterFrame:Boolean = false;
		private var _originalMouseY:Number;
		private var _isPress:Boolean;
		private var _step:int;
		private var _rect:Rect = new Rect();
		private var _gy:Number;
		private var _lastEnterFramerTime:Number = 0;
		private var _delayCall:DelayCall;
		private var _originalContentY:Number;
		private var _speedRadio:Number = 10;
		private static var _isDrag:Boolean = false;
		public function InvisibleScroll(content:DisplayObject = null, mask:Shape = null) {
			addEventListener(Event.ADDED_TO_STAGE, onAddToStage, false, 0, true);
			content = content;
			_mask = mask;
			init();
		}
		
		private function onAddToStage(e:Event):void {
			removeEventListener(Event.ADDED_TO_STAGE, onAddToStage);			
			MouseUtils.stage = stage;
		}
		
		private function init():void {
			setChildren();
			addChildren();
			addListeners();
		}
		
		public function set contentMask(value:Shape):void {
			_mask = value;
		}
		
		private function addChildren():void{
			addChild(_rect);
		}
		
		private function setChildren():void{
			_rect.alpha = 0;
		}
		
		private function addListeners():void {
			addEventListener(MouseEvent.MOUSE_DOWN, onMouseDown);
			addEventListener(MouseEvent.MOUSE_OVER, onMouseOver);
			addEventListener(MouseEvent.MOUSE_OUT, onMouseOut);
		}
	
		
		private function onMouseOut(e:MouseEvent):void {
			if (!_isPress) {
				MouseUtils.back();
			}
		}
		
		private function onMouseOver(e:MouseEvent):void { 
			if(_content.height>_mask.height){
				var shape:Shape = new UpDownArrowShape();
				MouseUtils.changeMouseShape(shape, -shape.width/2, -shape.height/2);
			}
		}
		
		
		private function onStageMouseUp(e:MouseEvent):void {
			stage.removeEventListener(MouseEvent.MOUSE_UP, onStageMouseUp);
			_isPress = false;
			if (!hitTestPoint(stage.mouseX, stage.mouseY)) {
				MouseUtils.back();
			}
			_isDrag = false;
		}
		
		private function onMouseDown(e:MouseEvent):void {
			if(_content.height > _mask.height){
				_originalMouseY = stage.mouseY;
				_originalContentY = _content.y;
				stage.addEventListener(MouseEvent.MOUSE_UP, onStageMouseUp);
				_isPress = true;
				_isDrag = true;
				addEnterFrame();
			}
		}
		private function addEnterFrame():void {
			if (!_isEnterFrame) {				
				dispatchEvent(new Event(CommonText.START));
				addEventListener(Event.ENTER_FRAME, onEnterFrame);
				_isEnterFrame = true
			}
		}
		private function onEnterFrame(e:Event):void {
			var offset:Number = - _content.getRect(_content).y;
			if (_isPress) {
				_gy = _originalContentY + (_originalMouseY - stage.mouseY) * _speedRadio;	
			}
			if (_gy > _mask.y + offset) {
				_gy = _mask.y + offset;
			}else if(_gy + _content.height< _mask.y + offset + _mask.height) {
				_gy = _mask.y - _content.height + offset + _mask.height;
			}
			if (_step > 1) {
				_gy = _mask.y - Math.round((_mask.y - _gy) / _step) * _step ; 
			}
			
			if (Math.abs(_gy - _content.y) < .3 && !_isPress) {
				_content.y = _gy;
				removeEnterFrame();
			}else {
				_content.y += (_gy - _content.y) / _easeSpeed;
			}			
			_lastEnterFramerTime = getTimer();
		}
		private function removeEnterFrame():void {
			removeEventListener(Event.ENTER_FRAME, onEnterFrame);
			_isEnterFrame = false;
			dispatchEvent(new Event(CommonText.STOP));
		}
			
		public function set content(value:DisplayObject):void {
			_content = value;
			if(stage){
				stage.addEventListener(MouseEvent.MOUSE_WHEEL, onContentMouseWheel, false, 0, true);
			}else {
				addEventListener(Event.ADDED_TO_STAGE, onAddedToStage, false, 0, true);
			}
		}
		
		static public function get isDrag():Boolean { return _isDrag; }
		
		public function get step():int { return _step; }
		
		public function set step(value:int):void {
			_step = value;
		}
		
		public function get isEnterFrame():Boolean { return _isEnterFrame; }
		
		public function get lastEnterFramerTime():Number { return _lastEnterFramerTime; }
		
		private function onAddedToStage(e:Event):void {
			removeEventListener(Event.ADDED_TO_STAGE, onAddedToStage);
			stage.addEventListener(MouseEvent.MOUSE_WHEEL, onContentMouseWheel, false, 0, true);
		}
		
		private function onContentMouseWheel(e:MouseEvent):void {
			if(stage){
				if(_mask.hitTestPoint(stage.mouseX,stage.mouseY)){
					if (_content.height > _mask.height) { 
						_gy = _content.y + 50 * e.delta;
						addEnterFrame();
					}
				}
			}
		}
	}
}