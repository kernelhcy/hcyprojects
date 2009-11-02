package com.ruochi.xml {
	import com.ruochi.utils.getRandom;
	public function randomXml(xml:XML):XML {
		var length:int = xml.*.length();
		for (var i:int = 0; i < length; i++) {	
			var randomId:int = getRandom(length - i);
			xml.insertChildAfter(xml.children()[length - 1], xml.children()[randomId]);
			delete xml.children()[randomId];
		}
		return xml
	}	
}