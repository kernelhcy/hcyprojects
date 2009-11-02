package com.ruochi.utils {
	public function deleteArrayElement(array:Array,element:Object):Array {
		var index = array.indexOf(element);
		array.splice(index, 1);
		return array;
	}
}