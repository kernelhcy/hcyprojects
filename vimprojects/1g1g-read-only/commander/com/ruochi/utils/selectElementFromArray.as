package com.ruochi.utils{
	public function selectElementFromArray(query:String, array:Array, length:int =-1):Array {
		if (query == "" || query == null) {
			if(length>0){
				return array.slice(0, length);return array;
			}else {
				return array;
			}
		}else{
			var returnArray:Array = new Array();
			var tempArray:Array = array.concat();
			var i:int =0;
			while (i < tempArray.length) { 
				if (String(tempArray[i]).toLowerCase().indexOf(query.toLowerCase()) == 0) {
					returnArray.push(tempArray.splice(i, 1));
					if (returnArray.length == length) {
						return returnArray;
					}
				}else {
					i++
				}
			}
			i = 0;
			while (i < tempArray.length) {
				if (String(tempArray[i]).toLowerCase().indexOf(query.toLowerCase()) >0) {
					returnArray.push(tempArray.splice(i, 1));
					if (returnArray.length == length) {
						return returnArray;
					}
				}else {
					i++
				}
			}
		}
		return returnArray
	}
}