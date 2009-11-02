package com.ruochi.display {
	import flash.display.DisplayObject;
	public function removeDisplayObject(displayObject:DisplayObject):void {
		if (displayObject && displayObject.parent) {
			displayObject.parent.removeChild(displayObject);
		}
	}	
}
	