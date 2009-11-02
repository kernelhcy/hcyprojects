package com.ruochi.string {
	public function trim(str:String):String {
		var pattern:RegExp = /^\s+|\s+$/g;
		return str.replace(pattern, "");
	}
}