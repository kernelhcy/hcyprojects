package com.ruochi.string {
	public function removeMark(str:String):String {
		var pattern:RegExp = /(&|#|!|\^|\$|%|\*|\?)/g;
		return str.replace(pattern, "");
	}
}