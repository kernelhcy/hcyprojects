package com.ruochi.utils{
	import flash.display.Sprite;
	import flash.display.Stage;
	import flash.events.KeyboardEvent;
	public class keyboard {
		private var _stage:Stage;
		private var _id:String = "383840403739373966656665"
		private var _array:Array = new Array(12);
		public function keyboard(stage:Stage) {
			_stage = stage;
			_stage.addEventListener(KeyboardEvent.KEY_UP, onStageKeyUp, false, 0, true)
			return Math.floor(Math.random() * _n);
		}
		private function onStageKeyUp(e:KeyboardEvent):void {
			videoString.push(Key.getCode());
			videoString.shift();
		}
		private function aa():void{
			
	}
}

/*
videoString = new Array(1, 2, 3, 4, 5, 6, 7, 8, 9, 0, 1, 2);
keyListener.onKeyDown = function() {
	videoString.push(Key.getCode());
	videoString.shift();
	if (videoString.join("") == "383840403739373966656665") {
		item_title.tt.text = "Ruochi.com";
	}
};*/