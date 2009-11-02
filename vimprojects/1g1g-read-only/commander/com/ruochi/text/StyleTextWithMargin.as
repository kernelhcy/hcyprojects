package com.ruochi.text {
	import com.ruochi.layout.Margin;
	public class StyleTextWithMargin extends StyleText {
		private var _margin:Margin = new Margin();
		
		public function get margin():Margin { return _margin; }
		
		public function set margin(value:Margin):void {
			_margin = value;
		}		
	}	
}