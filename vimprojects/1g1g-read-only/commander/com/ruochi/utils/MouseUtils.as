package com.ruochi.utils {
	import flash.display.DisplayObject;
	import flash.display.Stage;
	import flash.events.MouseEvent;
	import flash.ui.Mouse;
	public class MouseUtils {
		private static var _stage:Stage;
		private static var _displayObject:DisplayObject;
		private static var _offsetX:int;
		private static var _offsetY:int;
		static public function changeMouseShape(displayObject:DisplayObject, offsetX:int = 0, offsetY:int = 0 ):void {
			back();
			_stage.addEventListener(MouseEvent.MOUSE_MOVE, onStageMouseMove);
			_displayObject = displayObject;
			_offsetX = offsetX;
			_offsetY = offsetY;
			setLayout();
			_stage.addChild(_displayObject);
			Mouse.hide();
		}
		
		static private function onStageMouseMove(e:MouseEvent):void {			
			setLayout();
			e.updateAfterEvent();
		}
		static private function setLayout():void {
			_displayObject.x = _stage.mouseX + _offsetX;
			_displayObject.y = _stage.mouseY + _offsetY;
		}
		static public function set stage(value:Stage):void {
			_stage = value;
		}
		static public function back():void {
			if(_displayObject && _displayObject.parent){
				_displayObject.parent.removeChild(_displayObject);	
			}
			if (_displayObject) {					
				_displayObject = null;
			}
			_offsetX = 0;
			_offsetY = 0; 
			_stage.removeEventListener(MouseEvent.MOUSE_MOVE, onStageMouseMove);
			Mouse.show();
		}
	}
	
}