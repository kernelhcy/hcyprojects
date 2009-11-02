package com.ruochi.net {
	import flash.net.URLLoader;
	import flash.net.URLRequest;
	import flash.events.IOErrorEvent;
	import flash.events.Event;
	public class URLLoaderWithInfo extends URLLoader {
		public var info:Object = new Object();
	}
}