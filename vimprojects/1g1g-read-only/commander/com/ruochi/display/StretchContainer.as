package com.ruochi.display {
	import com.ruochi.layout.Margin;
	import flash.display.Sprite;
	import flash.display.DisplayObject;
	public class StretchContainer extends ElementContainer {
		public function StretchContainer() {
			
		}
		public function updateLayout():void {
			for (var i:int = 0; i < numChildren; i++) {
				setItemLayout(i);				
			}
		}
		
		private function setItemLayout(id:int):void {
			var displayObject:DisplayObject = getChildAt(id);
			if(displayObject){
				var margin:Margin = displayObject["margin"];
				if (margin) {
					margin = new Margin();					
				}
				if (margin.left.isAuto && margin.right.isAuto) {
					displayObject.x = (width - displayObject.width) / 2;
				}else if (margin.left.isAuto) {
					displayObject.x = width - margin.right.value - displayObject.width;
				}else if (margin.right.isAuto) {
					displayObject.x = margin.left.value;
				}else {
					displayObject.x = margin.left.value;
					displayObject.width = width - margin.left.value - margin.right.value;
				}
				if (margin.top.isAuto && margin.bottom.isAuto) {
					displayObject.y = (height - displayObject.height) / 2;
				}else if (margin.top.isAuto) {
					displayObject.y = height - margin.bottom.value - displayObject.height;
				}else if (margin.bottom.isAuto) {
					displayObject.y = margin.top.value;
				}else {
					displayObject.y = margin.top.value;
					displayObject.height = height - margin.top.value - margin.bottom.value;
				}
			}
		}
				
		override public function addChildAt(child:DisplayObject, index:int):DisplayObject {
			var displayObject:DisplayObject = super.addChildAt(child, index);
			setItemLayout(index);
			return displayObject;
		}
		
		override public function removeChildAt(index:int):DisplayObject {
			var displayObject:DisplayObject = super.removeChildAt(index);
			setItemLayout(index);
			return displayObject;
		}
		
		override public function addChild(child:DisplayObject):DisplayObject {
			var displayObject:DisplayObject = super.addChild(child);
			setItemLayout(numChildren-1);
			return displayObject;
		}
		
		/*override public function removeChild(child:DisplayObject):DisplayObject {
			var displayObject:DisplayObject = super.removeChild(child);
			var index:int = getChildIndex(child);
			return displayObject;
		}*/
	}	
}