package com.ruochi.component {
	import com.ruochi.display.FrameSprite;
	import com.ruochi.shape.radio.RadioCheckedShape;
	import com.ruochi.shape.radio.RadioDefaultShape;
	import com.ruochi.shape.Rect;
	import flash.display.Sprite;
	import com.ruochi.text.StyleText;
	import com.ruochi.layout.setCenter;
	public class RadioBtn extends Sprite{
		private var _clickArea:Rect = new Rect();
		private var _icon:FrameSprite = new FrameSprite();		
		private var _styletText:StyleText = new StyleText;
		private var _text:String;
		private var _width:int;
		public function RadioBtn() {
			init();
		}
		
		private function init():void {
			mouseChildren = false;
			buttonMode = true;
			_icon.addChild(new RadioDefaultShape());
			_icon.addChild(new RadioCheckedShape());
			addChild(_icon);
			addChild(_clickArea);
			_clickArea.width = 14;
			_clickArea.height = 20;
			_clickArea.alpha = 0;
			setCenter(_icon, _clickArea);
		}
		
		public function get icon():FrameSprite { return _icon; }
		
		public function set icon(value:FrameSprite):void {
			_icon = value;
		}
		
		public function get text():String { return _text; }
		
		public function set text(value:String):void {
			_text = value;
			_styletText.text = value;
			_styletText.wordWrap = true;
			addChild(_styletText);
			_styletText.x = _clickArea.x + _clickArea.width + 2;
			_width = _styletText.x + _styletText.textWidth; 
		}
		
		public function get styletText():StyleText { return _styletText; }
		
		public function set styletText(value:StyleText):void {
			_styletText = value;
		}
		
		override public function get width():Number { return _width; }
		
		override public function set width(value:Number):void {
			_width = value;
			_styletText.width = _width - _styletText.x;
		}
		
	}
	
}