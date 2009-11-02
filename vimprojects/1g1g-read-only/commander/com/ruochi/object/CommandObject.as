package com.ruochi.object {
	public class CommandObject {
		private var _name:String;
		private var _command:Function;
		private var _params:Array;
		public function CommandObject(commandName:String=null,commadnFunction:Function=null,commandParams:Array = null) {
			_name = commandName;
			_command = commadnFunction;
			_params = commandParams;
		}
		public function run():void {
			_command.apply(this, _params);
		}
		
		public function get name():String { return _name; }
		
		public function set name(value:String):void {
			_name = value;
		}
		
		public function get command():Function { return _command; }
		
		public function set command(value:Function):void {
			_command = value;
		}
		
		public function get params():Array { return _params; }
		
		public function set params(value:Array):void {
			_params = value;
		}
	}
}