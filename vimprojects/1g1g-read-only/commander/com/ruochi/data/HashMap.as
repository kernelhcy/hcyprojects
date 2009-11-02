package com.ruochi.data {
	public class HashMap extends Object {
		private var _length:int = 0;
		private var _object:Object = new Object();
		public function HashMap() {
			
		}
		public function get length():int {
			return _length;
		}
		public function put(key:String,value:Object):void {
			if (!hasKey(key)) {				
				_length++;
			}
			_object[key] = value;
		}
		public function remove(key:String):void {
			_object[key] = null;
			delete _object[key];
			_length--;
		}
		public function select(key:String):Object {
			return _object[key];
		}
		public function clear():void { 
			_object = new Object();
		}
		public function hasKey(key:String):Boolean {
			return _object[key] != null;
		}
		public function get object():Object {
			return _object;
		}
	}
}
