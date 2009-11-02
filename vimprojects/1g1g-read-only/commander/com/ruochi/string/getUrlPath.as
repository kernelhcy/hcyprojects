package com.ruochi.string {
	public function getUrlPath(str:String):String {
		if (str.substr(0, 7).toLowerCase() == "http://") {
			return str.substr(7).split("/").slice(1).join("/");
		} else {
			return str;
		}
		
	}
}