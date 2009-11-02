package com.ruochi.utils{
	public function assign(unknownValue,defaultValue):String {
		if (unknownValue==null||unknownValue==undefined) {
			return String(defaultValue);
		} else {
			return String(unknownValue);
		}
	}
}