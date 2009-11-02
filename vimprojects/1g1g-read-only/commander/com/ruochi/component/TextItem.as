package com.ruochi.component {
	import com.ruochi.display.ElementContainer;
	import com.ruochi.layout.Margin;
	import flash.display.Sprite;
	import com.ruochi.text.StyleText;
	import com.ruochi.shape.Rect;
	public class TextItem extends ElementContainer {
		protected var _styleText:StyleText = new StyleText();
		protected var _bg:Rect = new Rect(300, 24);
		protected var _margin:Margin = new Margin("2 5");
		public function TextItem() {
			init();
		}
		private function init():void {
			setChildren();
			addChildren();
		}
		
		private function addChildren():void{
			addChild(_bg);
			addChild(_styleText);
			height = _bg.height;
		}
		private function setChildren():void {
			_styleText.mouseEnabled = false;
			_styleText.x = _margin.left.value;
			_styleText.y = _margin.top.value;
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
			_bg.width = value;
		}
		
		override public function get height():Number { return super.height; }
		
		override public function set height(value:Number):void {
			super.height = value;
			_bg.height = value;
		}
	}
}