package com.ruochi.component{
	import flash.display.Sprite;
	import flash.display.Shape;
	import flash.events.Event;
	import com.ruochi.shape.Rect;
	import com.ruochi.shape.LoaderShape;
	import flash.events.MouseEvent;
	import flash.filters.DropShadowFilter;
	public class MouseCircleProgressBar extends CircleProgressBar {
		private static var _instance:MouseCircleProgressBar = new MouseCircleProgressBar();
		public function MouseCircleProgressBar(_r1:Number = 6, _r2:Number = 10) {
			if (!_instance) {
				super(_r1, _r2);
				_speed = 15;
			}else {
				throw new Error("singleton");
			}
		}
		override protected function buildUI():void {
			super.buildUI();
			_circle.filters = [new DropShadowFilter(3, 45, 0, .3)];
		}
		private function onStageMouseMove(e:MouseEvent):void {
			if (!visible) {
				visible = true;
			}
			setPosition();
			e.updateAfterEvent();

		}
		override public function start():void {
			setPosition();
			if (parent) {
				parent.addChild(this);
			}
			stage.addEventListener(MouseEvent.MOUSE_MOVE, onStageMouseMove, false, 0, true);
			stage.addEventListener(Event.MOUSE_LEAVE, onStageMouseLeave, false, 0, true);
			super.start();
		}	
		
		private function onStageMouseLeave(e:Event):void {
			if (_isStart) {
				visible = false;
			}
		}
		private function setPosition():void {
			x = stage.mouseX + 20;
			y = stage.mouseY - 0;
		}
		override public function stop():void {
			stage.removeEventListener(MouseEvent.MOUSE_MOVE, onStageMouseMove);
			super.stop();
		}
		public static function get instance():MouseCircleProgressBar {
			return _instance;
		}
	}
}