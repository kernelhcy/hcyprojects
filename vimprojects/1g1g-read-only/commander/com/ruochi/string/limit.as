package com.ruochi.string {
	public function limit(str:String, length:int):String {
		if(str && str.length>0){
			if(str.length>length){
				return str.slice(0, length) + "..";
			}else {
				return str;
			}
		}else {
			return str;
		}
	}
}