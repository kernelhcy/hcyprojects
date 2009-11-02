package com.ruochi.utils{
	import flash.display.DisplayObject;
	public function fillExactFit(displayObject:DisplayObject, displayObjectContainer:DisplayObject):void {
		displayObject.x = displayObjectContainer.x;
		displayObject.y = displayObjectContainer.y;
		displayObject.width = displayObjectContainer.width;
		displayObject.height = displayObjectContainer.height;
	}
}