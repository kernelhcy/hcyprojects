package com.ruochi.utils{
	import flash.display.*;
	public function isLocal(_displayObject:DisplayObject):Boolean {
		var urlStr:String = _displayObject.loaderInfo.loaderURL;
		if(urlStr.indexOf("ile:///")>0){	 
			return true;
		}else{
			return false;
		}
	}
}