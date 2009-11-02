package com.ruochi.xml {
	import flash.utils.describeType;
	public function objectToXml(ob:Object,joinChar:String=","):XML {
		var xml:XML = describeType(ob);	
		if(xml){
			var xmlList:XMLList = xml.variable;
			var length:int = xmlList.length();
			var i:int;
			var type:String;
			var name:String;
			var prop:String;
			for (i = 0; i < length; i++) {
				type = xmlList[i].@type;
				name = xmlList[i].@name;
				if(ob[name]!=null){
					if (type == "String" || type == "int" || type == "Number" || type == "uint" || type == "XML") {
						xmlList[i] = (ob[name]).toString();
					}else if (type == "Boolean") {
						if(ob[name] == true){
							xmlList[i] = "true"; 
						}else {
							xmlList[i] = "false";
						}
					}else if (type == "Array") {
						xmlList[i] = (ob[name] as Array).join(joinChar);
					}else if (type == "Object") {
						xmlList[i].setChildren(objectToXml(ob[name]).*);
					}else {
						xmlList[i] = ob[name]["toString"]();
					}
				}
			}
		}
		xml = new XML("<object/>");
		xml.setChildren(xmlList);
		for (prop in ob) { 
			var xmlNode:XML = new XML("<variable/>");
			xmlNode.@name = prop;
			if (ob[prop] is String) {
				xmlNode.@type = "String";
				xmlNode.setChildren(ob[prop]);
			}else if (ob[prop] is uint){
				xmlNode.@type = "uint";
				xmlNode.setChildren(ob[prop]);
			}else if (ob[prop] is int){
				xmlNode.@type = "int";
				xmlNode.setChildren(ob[prop]);
			}else if (ob[prop] is Number){
				xmlNode.@type = "Number";
				xmlNode.setChildren(ob[prop]);
			}else if (ob[prop] is Boolean){
				xmlNode.@type = "Boolean";
				xmlNode = new XML(ob[prop]);
			}else if (ob[prop] is XML){
				xmlNode.@type = "XML";
				xmlNode.setChildren(ob[prop]);
			}else if (ob[prop] is Array){
				xmlNode.@type = "Array";
				xmlNode = new XML((ob[prop] as Array).join(joinChar));
			}else {
				xmlNode.@type = "Object";
				xmlNode = objectToXml(ob[prop]);
			}
			xml.appendChild(xmlNode);
		}		
		return xml;
	}
}