package com.ruochi.component{
	import com.ruochi.shape.RoundRect;
	import com.ruochi.text.StyleText;
	import flash.display.Sprite;
	import flash.display.Stage;
	import flash.events.KeyboardEvent;
	import flash.text.TextField
	import flash.events.Event;
	import flash.events.MouseEvent;
	import com.ruochi.layout.setCenter;
	public class SimpleInfo extends Sprite{
		private var _styleText:StyleText = new StyleText;
		private var _bg:RoundRect = new RoundRect(200,200,5,0x000000);
		private static var _instance:SimpleInfo = new SimpleInfo();
		public function SimpleInfo() {
			addEventListener(Event.ADDED_TO_STAGE, onAddToStage, false, 0, true);
		}		
		private function onAddToStage(e:Event):void {
			init();
		}		
		private function init():void{
			setChildren();
			addChildren();
			addListener();
		}		
		
		private function addChildren():void{
			addChild(_bg);
			addChild(_styleText);
		}
		
		private function setChildren():void {
			close();
			_bg.alpha = .5;
			_styleText.x = 10;
			_styleText.y = 5;
		}
		
		private function setLayout():void {
			_bg.width = _styleText.width + 20;
			_bg.height = _styleText.height +10;
			setCenter(this, stage);
		}
		
		private function addListener():void{
			stage.addEventListener(KeyboardEvent.KEY_UP, onStageKeyUp, false, 0, true)
			stage.addEventListener(MouseEvent.CLICK, onStageClick, false, 0, true);
			stage.addEventListener(Event.RESIZE, onStageResize, false, 0, true);
		}		
		
		private function onStageResize(e:Event):void {
			setLayout();			
		}
		private function removeListener():void {
			if(stage){
				stage.removeEventListener(KeyboardEvent.KEY_UP, onStageKeyUp)
				stage.removeEventListener(MouseEvent.CLICK, onStageClick);
				stage.removeEventListener(Event.RESIZE, onStageResize);
			}
		}	
		private function onStageClick(e:MouseEvent):void {
			close();
		}
		
		private function onStageKeyUp(e:KeyboardEvent):void {
			close();
		}
		
		public function set text(txt:String):void {
			_styleText.text = txt;
			setLayout();
			open();			
		}
		
		public function open():void {
			addListener()
			visible = true;
		}
		
		public function close():void {
			visible = false;
			removeListener();
		}
		public static function get instance():SimpleInfo {
			return _instance;
		}
	}
}