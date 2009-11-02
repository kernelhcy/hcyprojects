package com.ruochi.component {
	import com.ruochi.net.SafeLoader;
	import flash.display.Sprite;
	import flash.events.Event;
	import flash.net.URLRequest;
	public class SpriteLoader extends Sprite {
		private var _safeLoader:SafeLoader = new SafeLoader();
		private var _url:String;
		public function SpriteLoader(src:String) {
			_url = src;
			init();
		}
		private function init():void  {
			_safeLoader.contentLoaderInfo.addEventListener(Event.COMPLETE, onLoaded,false,0,true);
			_safeLoader.load(new URLRequest(_url));
		}
		private function onLoaded(e:Event):void  {
			_safeLoader.contentLoaderInfo.removeEventListener(Event.COMPLETE, onLoaded);
			addChild(e.target.loader.content);
			dispatchEvent(new Event(Event.COMPLETE));
		}
	}	
}