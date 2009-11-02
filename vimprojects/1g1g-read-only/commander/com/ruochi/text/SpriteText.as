package com.ruochi.text {
	import flash.display.Sprite;
	
	public class SpriteText extends Sprite{
		private var _styleText:StyleText = new StyleText();
		public function SpriteText() {
			init();
		}
		
		private function init():void {
			mouseChildren = false;
			addChild(_styleText);
		}
		
		public function get styleText():StyleText { return _styleText; }
		
		public function set styleText(value:StyleText):void {
			_styleText = value;
		}
		
	}
	
}