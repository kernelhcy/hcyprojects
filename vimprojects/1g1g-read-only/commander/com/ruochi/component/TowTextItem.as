package com.ruochi.component {
	import flash.display.Sprite;
	import com.ruochi.text.StyleText;
	public class TowTextItem extends Sprite {
		private var _leftText:StyleText = new StyleText();
		private var _rightText:StyleText = new StyleText();
		
		public function get leftText():StyleText { return _leftText; }
		
		public function get rightText():StyleText { return _rightText; }
		
		public function TowTextItem() {
			addChild(_leftText);
			addChild(_rightText);
		}
	}
}