package com.ruochi.component {
	import flash.display.Sprite;
	import com.ruochi.display.FrameSprite;
	import com.ruochi.shape.Rect;
	import com.ruochi.text.StyleText;
	import flash.events.MouseEvent;
	import flash.events.Event;
	import com.ruochi.shape.checkBox.CheckBoxCheckedShape;
	import com.ruochi.shape.checkBox.CheckBoxDefaultShape;
	public class CheckBox extends Sprite {
		private var _styletText:StyleText = new StyleText;
		private var _icon:FrameSprite = new FrameSprite()
		private var _bg:Rect = new Rect();
		private var _isChecked:Boolean;
		private var _width:int = 100;
		public function CheckBox() {
			init();
		}
		
		private function init():void{
			setChildren();
			addChildren();
			addListener();
		}
		
		private function addListener():void{
			addEventListener(MouseEvent.CLICK, onClick, false, 0, true);
		}
		
		override public function get width():Number { return _width; }
		
		override public function set width(value:Number):void {
			_width = value;
			_styletText.width = _width -_styletText.x;			
			_bg.width = _styletText.x + _styletText.width;
			_bg.height = _styletText.height;
		}
		
		private function onClick(e:MouseEvent):void {
			_isChecked = !_isChecked;
			setIcon();
			dispatchEvent(new Event(Event.CHANGE));
		}
		
		private function setIcon():void {
			_icon.frame = _isChecked ? 2:1;
		}
		
		private function addChildren():void{
			addChild(_bg);
			addChild(_icon);
			addChild(_styletText);
			_icon.addChild(new CheckBoxDefaultShape());
			_icon.addChild(new CheckBoxCheckedShape());
		}
		
		private function setChildren():void{
			_icon.y = 5;
			_styletText.x = 12;
			_styletText.mouseEnabled = false;
			_bg.alpha = 0;
			buttonMode = true;
			_icon.mouseEnabled = false;
			_icon.mouseChildren = false;
			width = _width;
		}
		
		public function set text(txt:String):void {
			_styletText.text = txt;
			width = _width;
		}		
		
		public function get styletText():StyleText { return _styletText; }
		
		public function get icon():FrameSprite { return _icon; }
		
		public function get isChecked():Boolean { return _isChecked; }
		
		public function set isChecked(value:Boolean):void {
			_isChecked = value;
			setIcon();
		}
	}	
}