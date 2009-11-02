package com.ruochi.component {
	import flash.display.Sprite;
	import com.ruochi.text.StyleText;
	import com.ruochi.shape.Rect;
	public class TextItemBase extends Sprite {
		protected var _styleText:StyleText = new StyleText();
		protected var _bg:Rect = new Rect(100, 24);
		protected var _width:int;
		protected var _height:int;
		public function TextItemBase() {
			init();
		}
		private function init():void {
			setChildren();
			addChildren();
		}
		
		private function addChildren():void{
			addChild(_bg);
			addChild(_styleText);
		}
		private function setChildren():void {
			
		}
		public function get bg():Rect {
			return _bg;
		}
		public function get styleText():StyleText {
			return _styleText;
		}
		override public function get width():Number { return _width; }
		
		override public function set width(value:Number):void {
			super.width = value;
		}
	}
}