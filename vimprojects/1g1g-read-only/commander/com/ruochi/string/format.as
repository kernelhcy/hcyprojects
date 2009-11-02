package com.ruochi.string {
	public function format(str:String,...params):String {
		var pattern:RegExp = /\{(\d)\}/g;
		var object:Object = pattern.exec(str);
		var s1:String;
		var s2:String;
		var s3:String;
		var id:int;
		var offset:int;
		while (object != null) {
			s1 = str.substring(0, object.index);
			s3 = str.substring(pattern.lastIndex);
			id = object[1];
			if (params[id]) {
				s2 = params[id];
			}else {
				s2 = "";
			}
			offset = s2.length - String(object[0]).length; 
			pattern.lastIndex += offset;
			str = s1 + s2 + s3;
			object = pattern.exec(str);
		}
		return str;
	}
}