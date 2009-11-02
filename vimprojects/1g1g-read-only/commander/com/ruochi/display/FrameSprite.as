package com.ruochi.display {
	import flash.display.DisplayObject;
	import flash.display.Sprite;
	import com.ruochi.display.DisplayArray;
	import com.ruochi.utils.deleteAll;
	import com.ruochi.display.ElementContainer;
	public class FrameSprite extends ElementContainer {
		private var _frame:int = 1;
		private var _displayArray:DisplayArray = new DisplayArray();
		public function FrameSprite() {
			
		}
		
		public function get frame():int { return _frame; }
		
		public function set frame(value:int):void {
			deleteAll(this);
			_frame = value;
			if (_frame > 0 && _frame < _displayArray.length+1){
				super.addChild(_displayArray.getItem(_frame - 1));
			}
		}
		
		override public function addChild(child:DisplayObject):DisplayObject {
			_displayArray.push(child);
			frame = _frame;
			return child;
		}
		
		override public function addChildAt(child:DisplayObject, index:int):DisplayObject {
			return addChild(child);
		}
	}	
}
	