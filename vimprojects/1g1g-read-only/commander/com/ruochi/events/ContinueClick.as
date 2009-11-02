package com.ruochi.events {
	import flash.display.Sprite;
	import flash.events.MouseEvent;
	import flash.utils.Timer;
	import flash.events.Event;
	import flash.events.TimerEvent;
	public class ContinueClick {
		private static var _sprite:Sprite
		private static var _timer:Timer = new Timer(100);
		
		static public function setContinueClick(sprite:Sprite):void  {
			sprite.addEventListener(MouseEvent.MOUSE_DOWN, onSpriteMouseDown, false, 0, true);
			sprite.addEventListener(MouseEvent.MOUSE_UP, onSpriteMouseUp, false, 0, true);
			sprite.addEventListener(MouseEvent.MOUSE_OUT, onSpriteMouseOut, false, 0, true);
			if(sprite.stage){
				sprite.stage.addEventListener(MouseEvent.MOUSE_UP, onSpriteMouseUp, false, 0, true);
			}else {
				sprite.addEventListener(Event.ADDED_TO_STAGE, onSpriteAddToStage, false, 0, true);
			}
		}
		
		static private function onSpriteMouseOut(e:MouseEvent):void {
			stop();
		}
		
		static private function onSpriteAddToStage(e:Event):void {
			(e.target as Sprite).stage.addEventListener(MouseEvent.MOUSE_UP, onSpriteMouseUp, false, 0, true);
		}
		
		static private function onSpriteMouseUp(e:MouseEvent):void {
			stop();
		}
		
		static private function stop():void {
			_timer.reset();
		}
		
		static private function onSpriteMouseDown(e:MouseEvent):void {
			_sprite = e.currentTarget as Sprite;
			_timer.addEventListener(TimerEvent.TIMER, onTimer, false, 0, true);
			_timer.start();
		}		
		
		static private function onTimer(e:TimerEvent):void {
			if (_timer.currentCount > 5) {
				if(_sprite.mouseEnabled){
					_sprite.dispatchEvent(new MouseEvent(MouseEvent.CLICK, true, false, 0, 0));
				}
			}
		}
	}
}