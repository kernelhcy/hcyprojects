package com.ruochi.utils{
	public function objectToKeyValueString(object:Object):String {
		var str:String = "";
		for (var prop:String in object) {
			str += "&" + prop +"="+ String(object[prop]);
		}
		return str.substr(1);
	}
}