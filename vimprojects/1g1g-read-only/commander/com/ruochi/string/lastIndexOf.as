package com.ruochi.string {
	public function lastIndexOf(str:String, query:String):int {
		var queryLength:int = query.length;
		for (var i:int = str.length - queryLength; i > -1; i--) {
			if (str.substr(i, queryLength) == query) {
				return i;
			}			
		}
		return -1;
	}
}