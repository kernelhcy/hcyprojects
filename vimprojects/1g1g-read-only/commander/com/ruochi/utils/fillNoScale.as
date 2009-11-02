package com.ruochi.utils{
	import flash.display.DisplayObject;
	public function fillNoScale(displayObject:DisplayObject, displayObjectContainer:DisplayObject):void {
		displayObject.x = Math.round((displayObjectContainer.width -displayObject.width) / 2);
		displayObject.y = Math.round((displayObjectContainer.height -displayObject.height) / 2);
	}
}