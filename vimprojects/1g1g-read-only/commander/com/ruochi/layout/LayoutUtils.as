package com.ruochi.layout {
	import flash.display.DisplayObject;
	import flash.display.Sprite;
	import flash.display.Stage;
	import flash.events.Event;
	
	public class LayoutUtils {		
		static public function setLeft(displayObject:DisplayObject, value:Number = 0, referenceObject:DisplayObject = null):void {
			if (referenceObject) {
				displayObject.x = int(referenceObject.x + value);
			}else{
				if(displayObject.parent){
					displayObject.x = int(value);
				}
			}
		}
		
		static public function setRight(displayObject:DisplayObject, value:Number = 0, referenceObject:DisplayObject = null):void {
			if (referenceObject) {
				displayObject.x = int(getDisplayObjectRightX(referenceObject) - value - displayObject.width);
			}else{
				if(displayObject.parent){
					var containerWidth:Number = getContainerWidth(displayObject); 
					displayObject.x = int(containerWidth - value - displayObject.width);
				}
			}
		}
		
		static public function horizontalStretch(displayObject:DisplayObject, left:Number=0, right:Number=0, referenceObject:DisplayObject = null):void {
			if (referenceObject) {
				displayObject.x = int(referenceObject.x + left);
				displayObject.width = int(referenceObject.width - left - right);
			}else{
				if (displayObject.parent) { 
					var containerWidth:Number = getContainerWidth(displayObject);
					displayObject.x = int(left)
					displayObject.width = int(containerWidth - left - right);
				}
			}
		}
		
		static public function setTop(displayObject:DisplayObject, value:Number=0, referenceObject:DisplayObject = null):void {
			if (referenceObject) {
				displayObject.y = int(referenceObject.y + value);
			}else{
				if(displayObject.parent){
					displayObject.y = value;
				}
			}
		}
		
		static public function setBottom(displayObject:DisplayObject, value:Number=0, referenceObject:DisplayObject = null):void {
			if (referenceObject) {
				displayObject.y = int(getDisplayObjectBottomY(referenceObject) - value);
			}else{
				if(displayObject.parent){
					var containerHight:Number = getContainerHeight(displayObject);
					displayObject.y = int(containerHight - value - displayObject.height);
				}
			}
		}
		
		static public function verticallStretch(displayObject:DisplayObject, top:Number=0, bottom:Number=0, referenceObject:DisplayObject = null):void {
			if (referenceObject) {
				displayObject.y = int(referenceObject.y + top);
				displayObject.height = int(referenceObject.height - top - bottom);
			}else{
				if(displayObject.parent){
					var containerHight:Number = getContainerHeight(displayObject);
					displayObject.y = int(top)
					displayObject.height = int(containerHight - top - bottom);
				}
			}
		}
		
		static public function setCenter(displayObject:DisplayObject, referenceObject:DisplayObject = null):void {
			if (referenceObject) {
				displayObject.x = int(referenceObject.x + (referenceObject.width - displayObject.width) / 2);
			}else{
				if (displayObject.parent) { 
					var containerWidth:Number = getContainerWidth(displayObject);
					displayObject.x = int((containerWidth - displayObject.width) / 2);
				}
			}
		}
		
		static public function setMiddle(displayObject:DisplayObject, referenceObject:DisplayObject = null):void {
			if (referenceObject) {
				displayObject.y = int(referenceObject.y + (referenceObject.height - displayObject.height) / 2);
			}else{
				if(displayObject.parent){
					var containerHight:Number = getContainerHeight(displayObject);
					displayObject.y = int((containerHight - displayObject.height) / 2);
				}
			}
		}
		static private function getContainerHeight(displayObject:DisplayObject):Number{
			var containerObject:DisplayObject = displayObject.parent;
			if (containerObject is Stage) {
				return (containerObject as Stage).stageHeight;
			}else{
				return containerObject.height;
			}	
		}
		
		static private function getContainerWidth(displayObject:DisplayObject):Number{
			var containerObject:DisplayObject = displayObject.parent;
			if (containerObject is Stage) {
				return (containerObject as Stage).stageWidth;
			}else{
				return containerObject.width;
			}
		}
		
		static public function getDisplayObjectRightX(displayObject:DisplayObject):Number {
			return displayObject.x + displayObject.width;
		}
		
		static public function getDisplayObjectBottomY(displayObject:DisplayObject):Number {
			return displayObject.y + displayObject.height;
		}
		
		static public function alginBottom(displayObject:DisplayObject, referenceObject:DisplayObject, distance:Number = 0):void {
			displayObject.y = int(getDisplayObjectBottomY(referenceObject) + distance);
		}
		
		static public function alginTop(displayObject:DisplayObject, referenceObject:DisplayObject, distance:Number = 0):void {
			displayObject.y = int(referenceObject.y - displayObject.height - distance);
		}
		
		static public function alginRight(displayObject:DisplayObject, referenceObject:DisplayObject, distance:Number = 0):void {
			displayObject.x = int(getDisplayObjectRightX(referenceObject) + distance);
		}
		
		static public function alginLeft(displayObject:DisplayObject, referenceObject:DisplayObject, distance:Number = 0):void {
			displayObject.x = int(referenceObject.x - displayObject.width - distance);
		}
		
		static public function fill(displayObject:DisplayObject, referenceObject:DisplayObject = null):void {
			place(displayObject, new Margin(), referenceObject);
		}
	
		static public function place(displayObject:DisplayObject, margin:Margin = null, referenceObject:DisplayObject = null ) :void {
			if (!margin) {
				margin = new Margin();
			}
			if (margin.left.isAuto && margin.right.isAuto) {
				setCenter(displayObject, referenceObject);
			}else if (margin.left.isAuto) {
				setRight(displayObject, margin.right.value, referenceObject);
			}else if (margin.right.isAuto) {
				setLeft(displayObject, margin.left.value, referenceObject);
			}else {
				horizontalStretch(displayObject, margin.left.value, margin.right.value, referenceObject);
			}
			if (margin.top.isAuto && margin.bottom.isAuto) {
				setMiddle(displayObject, referenceObject);
			}else if (margin.top.isAuto) {
				setBottom(displayObject, margin.bottom.value, referenceObject);
			}else if (margin.bottom.isAuto) {
				setTop(displayObject, margin.top.value, referenceObject);
			}else {
				verticallStretch(displayObject, margin.top.value, margin.bottom.value, referenceObject); 
			}			
		}
	}
}
