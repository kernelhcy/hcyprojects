package com.ruochi.net {
	import flash.display.Loader;
	import flash.net.URLRequest;
	import flash.events.IOErrorEvent;
	import flash.events.Event;
	import flash.system.LoaderContext;
	import com.ruochi.events.Eventer;
	public class SafeLoader extends Loader {
		private var _url:String;
		private var _tryTime:int = 0;	
		public static var tryMax:int = 1;	
		public function SafeLoader() {			
			super();
			contentLoaderInfo.addEventListener(IOErrorEvent.IO_ERROR , errorHandler, false, 0, true);
		}
		override public function load(request:URLRequest, context:LoaderContext = null):void {
			if(_tryTime<tryMax){
				_url = request.url;
				try {
					super.load(request, context);
				}catch(e:Error) {
					tryAgain();
				}
			}else {
				throw new Error("out of try max time")
			}
		}
		private function errorHandler(e:IOErrorEvent):void {
			tryAgain();
		}
		private function tryAgain():void {			
			_tryTime++
			if(_tryTime<tryMax){
				load(new URLRequest(_url));				
			}else {
				dispatchEvent(new Event(Eventer.ERROR));
			}
		}
	}
}