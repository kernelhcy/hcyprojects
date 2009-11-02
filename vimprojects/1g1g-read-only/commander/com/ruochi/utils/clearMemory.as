package com.ruochi.utils {
	import flash.net.LocalConnection;
	public function clearMemory():void {
		try {
			new LocalConnection().connect('clearMemory');
			new LocalConnection().connect('clearMemory');
		} catch (e:Error) {
			
		}
	}
}