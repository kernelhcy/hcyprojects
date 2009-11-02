package com.ruochi.utils {
	public function addUniqueElementToArray(element:Object,array:Array):Array{
		if (array.indexOf(element) == -1) {
			array.push(element);
		}
		return array;
	}
}