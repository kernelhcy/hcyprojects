package com.ruochi.utils.xml{
	public function  xmlToVar(xml:XML, ob:Object) {
		var xmlList:XMLList = xml.*;
		for (var i:int = 0; i < xmlList.length(); i++) {
			ob[xmlList[i].name()] = xmlList[i];
		}
	}
}