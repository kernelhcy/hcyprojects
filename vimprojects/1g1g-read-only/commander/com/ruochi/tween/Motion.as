package com.ruochi.tween {
	import com.ruochi.geom.Color;
	import flash.display.DisplayObject;
	import flash.display.Sprite;
	import flash.events.EventDispatcher;
	import flash.utils.Dictionary;
	import flash.events.Event;
	import flash.utils.getTimer;
	import com.ruochi.utils.getObjectNumChildren;
	public class Motion extends EventDispatcher{
		private static var _dictionary:Dictionary = new Dictionary(true);
		private static var _isEnterFrame:Boolean;
		private static var _sprite:Sprite = new Sprite();
		private static var _objectNum:int;
		private var _object:Object;
		private var _endTime:int;
		private var _beginTime:int;
		private var _duration:Number;
		private var _easeFunction:Function;
		private var _keyValue:Object;
		private var _propNum:int = 0;
		private var _info:Object;
		private var _isRunning:Boolean;
		public function Motion(object:Object, keyValue:Object, duration:Number, easeFunction:Function) {
			_isRunning = true;
			_object = object;
			_beginTime = getTimer();
			_duration = duration*1000;
			_endTime = _beginTime + _duration;
			_keyValue = keyValue;
			_easeFunction = easeFunction;
			var tweenObject:TweenObject;
			if (_dictionary[_object] == undefined) {
				_dictionary[_object] = new Dictionary(true);
			}
			for (var prop:String in _keyValue) {
				_dictionary[_object][prop] = new TweenObject(_object, prop, _keyValue[prop], this);
				_propNum++;
			}
		}		
		public static function to(object:Object, keyValue:Object, duration:Number = 0.5, easeFunction:Function = null ):Motion {			
			if (easeFunction == null) {
				easeFunction = easeOut;
			}
			start();
			var motion:Motion = new Motion(object, keyValue, duration, easeFunction);						
			return motion;
		}
		
		static public function start():void {
			if (!_isEnterFrame) {
				_isEnterFrame = true;
				_sprite.addEventListener(Event.ENTER_FRAME, onStageEnterFrame, false, 0, true);
			}
		}		
		
		private function removeProp(propName:String):void {
			delete _dictionary[_object][propName];
			_propNum--;
			if (_propNum == 0) {
				dispatchEvent(new Event(Event.COMPLETE));
				_isRunning = false;
			}
		}
		
		static private function onStageEnterFrame(e:Event):void {
			var tweenObject:TweenObject;
			var motion:Motion;
			var nowTime:int;
			var prop:String;
			var tweenObj:Object;
			var propNum:int;
			_objectNum = 0;
			for (var motionObject:Object in _dictionary) {
				propNum = 0;
				for each(tweenObj in _dictionary[motionObject]) {
					tweenObject = tweenObj as TweenObject;
					motion = tweenObject._motion;
					nowTime = getTimer();
					if (nowTime < motion._endTime) {
						var t:int = nowTime - motion._beginTime;
						if (tweenObject._isColorTween) {
							var colorB:uint =  motion._easeFunction(t, tweenObject._colorBeginB, tweenObject._colorChangeB, motion._duration);
							var colorG:uint =  motion._easeFunction(t, tweenObject._colorBeginG, tweenObject._colorChangeG, motion._duration);
							var colorR:uint =  motion._easeFunction(t, tweenObject._colorBeginR, tweenObject._colorChangeR, motion._duration);
							tweenObject._object[tweenObject._prop] = ((((colorR) << 8) | colorG) <<8) | colorB
						}else {
							tweenObject._object[tweenObject._prop] = motion._easeFunction(t, tweenObject._beginValue, tweenObject._changeValue, motion._duration);
						}						
					}else {
						tweenObject._object[tweenObject._prop] = tweenObject._endValue;
						motion.removeProp(tweenObject._prop);
					}
					propNum++
				}
				if (propNum == 0) {
					delete _dictionary[motionObject];
				}
				_objectNum++; 
			}
			if (_objectNum == 0) {
				_sprite.removeEventListener(Event.ENTER_FRAME, onStageEnterFrame);
				_isEnterFrame = false; 
			}
		}
		
		public function stop():void {
			for (var prop:String in _keyValue) {
				if(_dictionary[_object] && _dictionary[_object][prop]){
					delete _dictionary[_object][prop]
				}
			}
			_isRunning = false;
		}
		
		public static function easeOut(t:Number, b:Number, c:Number, d:Number):Number {
			return -c * (t /= d) * (t - 2) + b;
		}
		
		public static function stopAllMotion():void {
			_sprite.removeEventListener(Event.ENTER_FRAME, onStageEnterFrame);
			_isEnterFrame = false;
			_dictionary = new Dictionary(true);
		}
		
		public function get object():Object { return _object; }
		
		public function get duration():Number { return _duration; }
		
		public function get info():Object { return _info; }
		
		public function set info(value:Object):void {
			_info = value;
		}
		
		public function get isRunning():Boolean { return _isRunning; }
		
	}	
}
import com.ruochi.tween.Motion;
class TweenObject{
	internal var _object:Object;
	internal var _prop:String;
	internal var _endValue:Number;
	internal var _beginValue:Number;
	internal var _colorBeginR:uint;
	internal var _colorBeginG:uint;
	internal var _colorBeginB:uint;
	internal var _colorChangeR:int;
	internal var _colorChangeG:int;
	internal var _colorChangeB:int;
	internal var _changeValue:Number;
	internal var _motion:Motion;
	internal var _isColorTween:Boolean;
	public function TweenObject(object:Object, prop:String, endValue:Number, motion:Motion) {
		_object = object;
		_prop = prop;
		_isColorTween = _prop.substr(0, 5) == "color";
		_beginValue = _object[prop];		
		_endValue = endValue;
		if (_isColorTween) {
			var beginColorValue:uint = _beginValue;
			_colorBeginB = beginColorValue & 0xff;
			beginColorValue = beginColorValue >>> 8;
			_colorBeginG = beginColorValue & 0xff;
			beginColorValue = beginColorValue >>> 8;
			_colorBeginR  = beginColorValue & 0xff;
			
			var endColorValue:uint = _endValue; 
			var colorEndB:uint = endColorValue & 0xff;
			endColorValue = endColorValue >>> 8;
			var colorEndG:uint = endColorValue & 0xff;
			endColorValue = endColorValue >>> 8;
			var colorEndR:uint  = endColorValue & 0xff;
			
			_colorChangeB = colorEndB - _colorBeginB;
			_colorChangeG = colorEndG - _colorBeginG;
			_colorChangeR = colorEndR - _colorBeginR;	
		}else {						
			_changeValue = _endValue - _beginValue;
		}
		_motion = motion;
	}
}
