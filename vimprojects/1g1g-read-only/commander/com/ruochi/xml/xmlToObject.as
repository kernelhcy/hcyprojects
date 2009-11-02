package com.ruochi.xml {
	import flash.utils.getDefinitionByName;
	public function xmlToObject(xml:XML, ob:Object):void {
		if(xml){
			var xmlList:XMLList = xml.variable;
			var length:int = xmlList.length();
			var i:int;
			var type:String;
			var name:String;
			var value:String;
			for (i = 0; i < length; i++) {
				type = xmlList[i].@type;
				name = xmlList[i].@name;
				if(String(xmlList[i]).length>0){
					value = String(xmlList[i]);
				}else {
					value = null;
					if (type == "Boolean") {
						ob[name] = false;
					}
				}
				if(value){
					if (type == "String" || type == "") {
						ob[name] = String(value);
					}else if (type == "uint") {
						ob[name] = uint(value);
					}else if (type == "int") {
						ob[name] = int(value);
					}else if (type == "Number") {
						ob[name] = Number(value);
					}else if (type == "uint") {
						ob[name] = uint(value);
					}else if (type == "Boolean") {
						if (value == "false") {
							ob[name] = false;
						}else {
							ob[name] = true;
						}
					}else if (type == "Array") {
						var splitChar:String;
						if(xmlList[i].@splitChar == undefined){
							splitChar = ",";
						}else {
							splitChar = xmlList[i].@splitChar;
						}
						ob[name] = value.split(splitChar);
					}else if (type == "Object") {
						xmlToObject(xmlList[i],ob[name]);
					}else {
						var myClass:Class = getDefinitionByName(type) as Class;
						ob[name] = myClass["parse"](value);
					}
				}
			}
		}
	}
}