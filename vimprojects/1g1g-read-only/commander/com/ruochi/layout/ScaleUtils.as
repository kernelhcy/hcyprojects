package com.ruochi.layout {
	public class ScaleUtils {		
		import flash.display.DisplayObject;
		public static const NO_SCALE:String = "noScale";
		public static const EXACT_FIT:String = "exactFit";
		public static const SHOW_ALL:String = "showAll";
		public static const NO_BORDER:String = "noBorder";
		public static function fillNoScale(displayObject:DisplayObject, containerWidth:Number, containerHeight:Number):void {
			displayObject.x = Math.round((containerWidth -displayObject.width) / 2);
			displayObject.y = Math.round((containerHeight -displayObject.height) / 2);
		}
		public static function fillExactFit(displayObject:DisplayObject, containerWidth:Number, containerHeight:Number):void {
			displayObject.width = containerWidth;
			displayObject.height = containerHeight;
		}
		public static function fillShowAll(displayObject:DisplayObject, containerWidth:Number, containerHeight:Number):void { 
			var originW:Number = displayObject.width / displayObject.scaleX;
			var originH:Number = displayObject.height / displayObject.scaleY;
			if (originW/originH>containerWidth/containerHeight) { 
				displayObject.width = containerWidth;	
				displayObject.height = Math.round(displayObject.width / originW * originH);
			} else {
				displayObject.height = containerHeight;
				displayObject.width = Math.round(displayObject.height / originH * originW);
			}
			displayObject.x = Math.round((containerWidth -displayObject.width) / 2);
			displayObject.y = Math.round((containerHeight -displayObject.height) / 2);
		}
		public static function fillNoBorder(displayObject:DisplayObject, containerWidth:Number, containerHeight:Number):void {
			var originW:Number = displayObject.width / displayObject.scaleX;
			var originH:Number = displayObject.height / displayObject.scaleY;
			if (originW/originH>containerWidth/containerHeight) {
				displayObject.height=containerHeight;
				displayObject.width=displayObject.height/originH*originW;
			} else {
				displayObject.width=containerWidth;
				displayObject.height=displayObject.width/originW*originH;
			}
			displayObject.x=(containerWidth-displayObject.width)/2;
			displayObject.y =(containerHeight-displayObject.height)/2;
			displayObject.x = Math.round((containerWidth -displayObject.width) / 2);
			displayObject.y = Math.round((containerHeight -displayObject.height) / 2);
		}
	}	
}