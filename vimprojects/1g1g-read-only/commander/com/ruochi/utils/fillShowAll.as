package com.ruochi.utils{
	import flash.display.DisplayObject;
	public function fillShowAll(displayObject:DisplayObject, displayObjectContainer:DisplayObject):void {
		var originW = displayObject.width/displayObject.scaleX;
		var originH = displayObject.height/displayObject.scaleY;
		if (originW/originH>displayObjectContainer.width/displayObjectContainer.height) {
			displayObject.width = displayObjectContainer.width;			
			displayObject.height = Math.round(displayObject.width / originW * originH);
		} else {
			displayObject.height=displayObjectContainer.height;
			displayObject.width = Math.round(displayObject.height / originH * originW);
		}
		displayObject.x = Math.round((displayObjectContainer.width -displayObject.width) / 2);
		displayObject.y = Math.round((displayObjectContainer.height -displayObject.height) / 2);
	}
}