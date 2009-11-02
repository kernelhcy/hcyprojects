package com.ruochi.utils.xml{
	public function attributeToVar(xml:XML,ob:Object) {
		for (var i:int = 0; i < xml.attributes().length(); i++) {
			try{
				if (xml.attributes()[i] == "true") {
					ob[String(xml.attributes()[i].name())] = true;
				}else if (xml.attributes()[i] == "false") {
					ob[String(xml.attributes()[i].name())] = false;
				}else{ 
					ob[String(xml.attributes()[i].name())] = xml.attributes()[i];
				}
			}catch (e:Error) {
				
			}
		}
	}
}