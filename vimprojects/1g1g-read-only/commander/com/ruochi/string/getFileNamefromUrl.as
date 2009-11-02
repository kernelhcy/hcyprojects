package com.ruochi.string {
	import com.ruochi.string.lastIndexOf;
	public function getFileNamefromUrl(str:String):String {
		var index:int = lastIndexOf(str, "/");
		if (index > 0) {
			return(str.substring(index));
		}else {
			return str;
		}
	}
}