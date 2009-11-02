package com.ruochi.utils{
	public function randomArray(_array:Array):Array {
		var tempArray:Array = new Array();
		do {
			var randomId:int=getRandom(_array.length);
			tempArray.push(_array[randomId]);
			_array.splice(randomId,1);
		} while (_array.length>0);
		return tempArray;
	}
}