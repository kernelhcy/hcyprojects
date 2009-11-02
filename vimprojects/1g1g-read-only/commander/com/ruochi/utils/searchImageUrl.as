package com.ruochi.utils{
	public function searchImageUrl(_str:String):String {
		var pattern:RegExp = /[iI][mM][gG][^>]+[sS][rR][c|C] *= *['"]?([^'" ]+)['"]?/;
		var resultArray:Array =  _str.match(pattern);
		if (resultArray!=null) {
			return resultArray[1];	
		}else {
			return null;
		}			
	}
}