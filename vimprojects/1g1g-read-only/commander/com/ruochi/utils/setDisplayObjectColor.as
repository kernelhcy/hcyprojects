package com.ruochi.utils {
	import flash.geom.ColorTransform;
	import flash.display.DisplayObject;
	public function setDisplayObjectColor(displayObject:DisplayObject , color:uint):void {
		var colorTransform:ColorTransform = new ColorTransform();
		colorTransform.alphaMultiplier = displayObject.alpha;
		colorTransform.color = color
		displayObject.transform.colorTransform = colorTransform;
	}
}