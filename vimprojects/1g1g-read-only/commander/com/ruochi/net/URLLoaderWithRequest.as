package com.ruochi.net {
	import flash.net.URLLoader;
	import flash.net.URLRequest;
	import flash.events.IOErrorEvent;
	import flash.events.Event;
	import flash.net.URLVariables;
	public class URLLoaderWithRequest extends URLLoader {
		private var _urlRequest:URLRequest;
		override public function load(request:URLRequest):void {
			_urlRequest = request;
			super.load(request);
		}
		override public function toString():String {
			return _urlRequest.url + "?" + (_urlRequest.data as URLVariables).toString();
		}
		
		public function get urlRequest():URLRequest { return _urlRequest; }
	}	
}