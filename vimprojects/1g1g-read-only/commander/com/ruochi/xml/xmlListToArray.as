package com.ruochi.utils {
	public function xmlListToArray(xmlList:XMLList):Array {
		var array:Array = new Array();
		var length:int = xmlList.length()
		for (var i:int; i < length; i++) {
			array.push(String(xmlList[i]))
		}
		return array;
	}
}