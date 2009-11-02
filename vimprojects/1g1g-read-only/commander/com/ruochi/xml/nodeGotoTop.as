package com.ruochi.utils.xml{
	public function nodeGotoTop(xml:XML,id:int) {
		var tempXmlList = new XMLList();
		var i:int = 0;
		for (i = 0; i < id; i++) {
			tempXmlList = tempXmlList + xml.children()[0];
			delete xml.children()[0];
		}
		xml.* = xml.*+tempXmlList;
	}
}