package com.ruochi.net {
	import flash.net.URLLoader;
	import flash.net.URLRequest;
	import flash.events.IOErrorEvent;
	import flash.events.Event;
	import flash.net.URLVariables;
	/**
	 * A 3-trial URL loader. Ensure that each SafeURLLoader only serves one URL; or else
	 * the trial counting may be confused.
	 */
	public class SafeURLLoader extends URLLoader {
		public static const IO_ALL_ERROR:String = "ioAllError";	// Denote all the trials got IO error;
		private var _tryTime:int = 0;
		private var _urlRequest:URLRequest;
		
		public function get url():String {
			if (_urlRequest.data) {
				return _urlRequest.url +"?" + (_urlRequest.data as URLVariables).toString()
			}else {
				return _urlRequest.url;
			}
		}
		
		public function get urlRequest():URLRequest { return _urlRequest; }
		
		private static var tryMax:int = 0;
		public function SafeURLLoader() {
			super();
			addEventListener(IOErrorEvent.IO_ERROR, errorHandler, false, 0, true);
		}
		public function safeLoad(request:URLRequest):void {
			_urlRequest = request;
			super.load(request);		// super avoids override of the load method by sub classes.
		}
		private function errorHandler(e:IOErrorEvent):void {
			if (_tryTime < tryMax) {
				super.load(_urlRequest);	// super avoids override of the load method by sub classes.
			} else {
				dispatchEvent(new Event(IO_ALL_ERROR));
			}
			_tryTime++; trace('xxxxxxxxxxxxxxxxxxxxxxxxxxxxx');
		}
	}
}