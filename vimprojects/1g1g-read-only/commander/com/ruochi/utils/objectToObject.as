package com.ruochi.utils{
	public function objectToObject(objectOld:Object, objectNew:Object):void {
		for (var prop:String in objectOld) {
			objectNew[prop] = objectOld[prop]
		}
	}
}