package com.ruochi.utils{
	import flash.display.DisplayObject;
	public function fillNoBorder(displayObject:DisplayObject, displayObjectContainer:DisplayObject):void {
		var originW = displayObject.width/displayObject.scaleX;
		var originH = displayObject.height/displayObject.scaleY;
		if (originW/originH>displayObjectContainer.width/displayObjectContainer.height) {
			displayObject.height=displayObjectContainer.height;
			displayObject.width=displayObject.height/originH*originW;
		} else {
			displayObject.width=displayObjectContainer.width;
			displayObject.height=displayObject.width/originW*originH;
		}
		displayObject.x=(displayObjectContainer.width-displayObject.width)/2;
		displayObject.y =(displayObjectContainer.height-displayObject.height)/2;
		displayObject.x = Math.round((displayObjectContainer.width -displayObject.width) / 2);
		displayObject.y = Math.round((displayObjectContainer.height -displayObject.height) / 2);
	}
}