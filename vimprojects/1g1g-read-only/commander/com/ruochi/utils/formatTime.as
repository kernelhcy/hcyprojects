package com.ruochi.utils {
	public function formatTime(n:Number):String {
		var mm:Number;
		var ss:Number;
		var mmStr:String;
		var ssStr:String;
		n = Math.floor(n);
		mm = Math.floor(n/60);
		ss = n % 60;
		mmStr = numToDoubleDigi(mm);
		ssStr = numToDoubleDigi(ss);
		return mmStr + ":" + ssStr;

	}
}