package com.ruochi.utils{
	public function getObjectNumChildren(object:Object):int {
		var i:int = 0;
		for (var prop:String in object) {
			i++;
		}
		return i;
	}
}