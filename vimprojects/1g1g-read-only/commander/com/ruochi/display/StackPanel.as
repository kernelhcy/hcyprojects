package com.ruochi.display {
	import flash.display.DisplayObject;
	import flash.display.Sprite;
	import com.ruochi.layout.Gap;
	public class StackPanel extends ElementContainer {
		public static const HORIZONTAL:String = "horizontal";
		public static const VERTICAL:String = "vertical";
		private var _type:String = VERTICAL;
		private var _gap:Gap = new Gap();
		
		
		public function updateLayout(index:int = 0):void {
			var displayObject:DisplayObject;
			var next:int;
			var gap:Number;
			var i:int;
			if (_gap.isAuto) {
				if(numChildren >1){
					if (_type == VERTICAL) {
						var h:Number = _height;
						for (i = 0; i < numChildren; i++) {
							h -= getChildAt(i).height;
						}
						gap = int(h / numChildren - 1);
					}else {
						var w:Number = _width;
						for (i = 0; i < numChildren; i++) {
							w -= getChildAt(i).width;
						}
						gap = int(w / numChildren - 1);
					}
				}else {
					gap = 0;
				}
			}else {
				gap = _gap.value
			}
			if (index == 0) {
				next = 0;
			}else {
				displayObject = getChildAt(index - 1);
				if (type == HORIZONTAL) {
					next = displayObject.x + displayObject.width;
				}else {
					next = displayObject.y + displayObject.height;
				}
			}
			for (i = index; i < numChildren; i++) {
				displayObject = getChildAt(i);
				if (type == HORIZONTAL) {
					displayObject.x = next 
					next = displayObject.x + displayObject.width + gap;
				}else {
					displayObject.y = next;
					next = displayObject.y + displayObject.height +gap; 
				}				
			}
		}		
				
		override public function addChildAt(child:DisplayObject, index:int):DisplayObject {
			var displayObject:DisplayObject = super.addChildAt(child, index);
			updateLayout(index);
			return displayObject;
		}
		
		override public function removeChildAt(index:int):DisplayObject {
			var displayObject:DisplayObject = super.removeChildAt(index);
			updateLayout(index);
			return displayObject;
		}
		
		override public function addChild(child:DisplayObject):DisplayObject {
			var displayObject:DisplayObject = super.addChild(child);
			updateLayout(numChildren-1);
			return displayObject;
		}
		
		override public function removeChild(child:DisplayObject):DisplayObject {
			var displayObject:DisplayObject = super.removeChild(child);
			var index:int = getChildIndex(child);
			return displayObject;
		}
		
		override public function get width():Number {
			return actualWidth;
			/*if (_type == VERTICAL) {
				return actualWidth;
			}else {
				
			}*/
		}
		
		override public function set width(value:Number):void {
			super.width = value;
		}
		
		override public function get height():Number { return actualHeight; }
		
		override public function set height(value:Number):void {
			super.height = value;
		}
		
		
		public function get type():String { return _type; }
		
		public function set type(value:String):void {
			_type = value;
		}
		
		public function get gap():Gap { return _gap; }
		
		public function set gap(value:Gap):void {
			_gap = value;
		}
	}	
}
	