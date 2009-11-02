package com.ruochi.utils {
	import com.ruochi.utils.isNum;
	public function  paramToVar(paramObject:Object, varObject:Object):void {
		for (var name:String in paramObject) {
			if (String(paramObject[name]).toLowerCase() == "true") {
				varObject[name] = true;
			}else if (String(paramObject[name]).toLowerCase() == "false") { 
				varObject[name] = false;
			}else if(isNum(paramObject[name])) {
				varObject[name] = Number(paramObject[name]);
			}else {
				varObject[name] = String(paramObject[name]);
			}
		}
	}
}