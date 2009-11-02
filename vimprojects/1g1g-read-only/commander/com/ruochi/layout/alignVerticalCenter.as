package com.ruochi.layout{
	import flash.display.DisplayObject;
	public function alignVerticalCenter(displayObject:DisplayObject,h:Number,y:Number=0):void {
		displayObject.y = Math.round((h - displayObject.height) / 2 + y);
	}
}
