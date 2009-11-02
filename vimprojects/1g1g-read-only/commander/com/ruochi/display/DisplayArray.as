package com.ruochi.display {
	import flash.display.DisplayObject;
	public dynamic class DisplayArray {
		private var _array:Array = new Array;
		public function insert(startIndex:int,ob:DisplayObject):void {
			_array.splice(startIndex, 0, ob);
		}
		
		public function push(ob:DisplayObject):void {
			_array.push(ob);
		}
		
		public function setIndex(oldIndex:int, newIndex:int):DisplayObject {
			var ob:DisplayObject = _array.splice(oldIndex, 1)[0] as DisplayObject; 
			insert(newIndex, ob);
			return ob;
		}
		public function getItemByName(str:String, startId:int = 0):DisplayObject {
			return _array[getItemIndexByName(str,startId)] as DisplayObject
		}
		
		public function getItemIndexByName(str:String, startId:int = 0):int {
			for (var i:int = startId; i < length; i++) {
				if ((_array[i] as DisplayObject).name == str) {
					return i;
				}
			}
			return -1;
		}
		public function getItemIndex(displayObject:DisplayObject):int {
			for (var i:int = 0; i < length; i++) {
				if (_array[i]== displayObject) {
					return i;
				}
			}
			return -1;
		}
		public function get length():int {
			return _array.length;
		}
		public function getItem(id:int):DisplayObject {
			return _array[id] as DisplayObject;
		}
		
		public function removeItem(displayObject:DisplayObject):void {
			removeItemAt(getItemIndex(displayObject));
		}
		public function removeItemAt(id:int):DisplayObject {
			return _array.splice(id, 1)[0] as DisplayObject;
		}
		public function setItem(displayObject:DisplayObject, id:int):void {
			_array[id] = displayObject;
		}
	}	
}