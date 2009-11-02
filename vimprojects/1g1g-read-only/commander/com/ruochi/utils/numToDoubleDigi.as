package com.ruochi.utils {
	public function numToDoubleDigi(num:int):String {
		if (num < 10) {
			return "0" + num;
		}else {
			return String(num);
		}
	}
}