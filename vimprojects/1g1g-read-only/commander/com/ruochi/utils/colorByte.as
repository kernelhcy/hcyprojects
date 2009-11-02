package com.ruochi.utils{
	public function colorByte(_int:int):int {
		if(_int<0){
			return _int+256;
		}else{
			return _int;
		}
	}
}