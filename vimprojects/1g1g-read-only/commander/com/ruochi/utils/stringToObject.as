package com.ruochi.utils{	
	public function stringToObject(str:String):*{
	var strArray:Array = str.split(".");
	var o:*
	for (var i:int = 0; i < strArray.length; i++) {
		o = o[strArray[i]]
	}
	return o
}