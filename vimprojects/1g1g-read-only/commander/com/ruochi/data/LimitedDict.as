package com.ruochi.data {
	
	public class LimitedDict {
		private var _maxLength:int = 10;
		private var _array:Array = new Array();
		private var _dict:Object = new Object();
		public function LimitedDict() {
			
		}
		
		public function getItemByName(value:String):Object {
			if(_dict[value]){
				_array.splice(_array.indexOf(value), 1)
				_array.push(value);
				return _dict[value];
			}
			return null;
			
		}
		
		public function deleteItemByName(value:String):void {
			if(_dict[value]){
				_array.splice(_array.indexOf(value), 1)
				delete _dict[value];
			}
		}
		
		public function setItem(key:String, value:Object):Object {
			_array.push(value);
			_dict[key] = value;
			while (_array.length > _maxLength) {
				delete _dict[_array.shift()];				
			}
			return _dict[key];
		}
		
		public function get maxLength():int { return _maxLength; }
		
		public function set maxLength(value:int):void {
			_maxLength = value;
		}
		
		//public function get array():Array { return _array; }
		
	}
	
}
