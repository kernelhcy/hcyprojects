package com.ruochi.utils{
	public function replaceAnd(_str:String) {
		var pattern:RegExp = /&/g;
		return _str.replace(pattern, "^");
	}
}