package com.ruochi.utils {
	public function timeStringToDate(str:String):Date {
		var array:Array = str.split(":");
		var data:Date = new Date(0);
		if (array.length>1) {
			data.minutes = Number(array[0]);
			data.seconds = Number(array[1]);
		}
		return data;
	}
}