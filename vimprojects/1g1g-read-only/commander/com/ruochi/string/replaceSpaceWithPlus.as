package com.ruochi.string{
	public function replaceSpaceWithPlus(_str:String):String {
		var pattern:RegExp = / +/g;
		return _str.replace(pattern, "+");
	}
}