package com.ruochi.utils {
	public function stringToNum(str:String):Number {
		if (str.indexOf("-") > 0) {
			return stringToDate(str).valueOf();
		}else {
			return Number(str);
		}
	}
}