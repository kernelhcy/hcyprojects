package com.ruochi.utils {
	public function stringToDate(str:String):Date {
		var array:Array = new Array(0, 0, 0, 0, 0, 0, 0);
		var strArray:Array = str.split("-");
		for (var i:int = 0; i < strArray.length; i++) {
			array[i] = Number(strArray[i]);
		}
		var data:Date = new Date(Number(array[0]), Number(array[1])-1, Number(array[2]), Number(array[3]), Number(array[4]), Number(array[5]), Number(array[6]));
		return data;
	}
}