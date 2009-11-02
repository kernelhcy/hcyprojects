package com.ruochi.utils.xml{
	public function getNodeDepth(_xml:XML):int {
		var tempXml=_xml;
		var depth:int=0;
		do {
			tempXml=tempXml.parent();
			depth++;
		} while (tempXml.parent()!=undefined);
		return depth;
	}
}