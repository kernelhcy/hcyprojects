package com.ruochi.events {
	import flash.display.InteractiveObject;
	import flash.display.DisplayObject;
	import flash.events.MouseEvent;
	import flash.geom.Point;
	import com.ruochi.utils.distance;
	public class SingleClick {
		public static const SINGLE_CLICK:String = "singleClick";
		private static var _singleClickDistance:Number = 5;
		private static var _startPoint:Point;
		public function SingleClick() {
			
		}
		
		public static function addSingleClickDispather(interactiveObject:InteractiveObject):void {
			interactiveObject.addEventListener(MouseEvent.MOUSE_DOWN, onMouseDown, false, 0, true);
			interactiveObject.addEventListener(MouseEvent.MOUSE_UP, onMouseUp, false, 0, true);
		}
		
		static private function onMouseUp(e:MouseEvent):void {
			var interactiveObject:InteractiveObject = e.currentTarget as InteractiveObject;
			if(_startPoint){
				if (distance(_startPoint.x,_startPoint.y, e.localX, e.localY) < _singleClickDistance) {
					interactiveObject.dispatchEvent(new MouseEvent(SINGLE_CLICK, e.bubbles, e.cancelable, e.localX, e.localY, e.relatedObject, e.ctrlKey, e.altKey, e.shiftKey, e.buttonDown, e.delta));
				}
			}
		}
		
		static private function onMouseDown(e:MouseEvent):void {
			_startPoint = new Point(e.localX, e.localY);
			var displayObject:DisplayObject = e.currentTarget as DisplayObject;
			if (displayObject) {
				if (displayObject.stage) {
					displayObject.stage.addEventListener(MouseEvent.MOUSE_UP, onStagMouseUp, false, 0, true);
				}
			}
		}
		
		static private function onStagMouseUp(e:MouseEvent):void {
			_startPoint = null;
		}		
	}	
}