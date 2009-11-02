package com.ruochi.xml{
	public function getNodeDepth(xml:XML):int {
		var tempXml:XML = xml;
		var depth:int = 0;
		while (tempXml.parent() != undefined) {
			tempXml=tempXml.parent();
			depth++;
		}
		return depth;
	}
}