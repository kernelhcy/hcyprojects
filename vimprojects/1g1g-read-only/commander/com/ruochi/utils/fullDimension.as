package com.ruochi.utils{
	import flash.display.DisplayObject;
	public function fullDimension(displayObject:DisplayObject,w:Number,h:Number):void {
		var originW = displayObject.width/displayObject.scaleX;
		var originH = displayObject.height/displayObject.scaleY;
		if (originW/originH>w/h) {
			displayObject.height=h;
			displayObject.width=displayObject.height/originH*originW;
		} else {
			displayObject.width=w;
			displayObject.height=displayObject.width/originW*originH;
		}
		displayObject.x=(w-displayObject.width)/2;
		displayObject.y =(h-displayObject.height)/2;
	}
}