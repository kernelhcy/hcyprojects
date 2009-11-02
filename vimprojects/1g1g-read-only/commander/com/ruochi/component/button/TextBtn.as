package com.ruochi.component.button{
	import com.ruochi.component.button.BtnBase;
	import com.ruochi.text.StyleText;
	import com.ruochi.layout.setCenter;
	import com.ruochi.utils.defaultNum;
	public class TextBtn extends BtnBase {
		private var _styleText:StyleText = new StyleText()
		private var _text:String;
		public function TextBtn(txt:String = "", w:Number = 48, h:Number = 24) {
			super(w, h);
			_text = txt;
			init()
		}
		public function init():void {
			initBase();
			buildUI()
		}
		private function buildUI():void {
			_styleText.text = _text;
			_styleText.mouseEnabled = false;
			_styleText.embedFonts = true;			
			setCenter(_styleText, bg);
			_styleText.y--;
			addChild(_styleText);
		}
		public function set text(txt:String):void {
			_text = txt
			init();
		}
		public function get styleText():StyleText {
			return _styleText;
		}
	}
}