package com.ruochi.utils.graphic {
	import flash.display.DisplayObject;
	import flash.geom.ColorTransform;
	public function fillColor(displaceObject:DisplayObject, col:Number):void {
		var colorTransform:ColorTransform = new ColorTransform();
		colorTransform.color = col;
		displaceObject.transform.colorTransform = colorTransform;
		colorTransform = null;
	}
}