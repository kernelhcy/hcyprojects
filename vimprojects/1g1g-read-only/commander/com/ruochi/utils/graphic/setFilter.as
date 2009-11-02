package com.ruochi.utils.graphic {
	import flash.display.DisplayObject;
	import flash.filters.BitmapFilter;
	public function setFilter(displaceObject:DisplayObject, filter:BitmapFilter):void {
        var myFilters:Array = new Array();
        myFilters.push(filter);
        displaceObject.filters = myFilters;
		myFilters = null;
	}
}