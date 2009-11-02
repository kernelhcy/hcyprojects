package com.ruochi.display {
	import flash.display.DisplayObject;
	import flash.display.Loader;
	import flash.display.LoaderInfo;
	import flash.display.Sprite;
	import flash.events.Event;
	import flash.net.URLRequest;
	import com.ruochi.utils.setDisplayObjectColor;
	import com.ruochi.utils.objectToObject;
	import flash.system.LoaderContext;
	import flash.system.SecurityDomain;
	public class Source extends Sprite implements ISource {
		private var _loader:Loader;
		private var _loaderInfo:LoaderInfo;
		private var _url:String;
		private var _color:uint;
		private var _isSetColor:Boolean = false;
		private var _displayObject:DisplayObject;
		private var _width:int;
		private var _height:int;
		//private var _initParams:Object;
		private var _content:Object = new Object();
		public var info:Object;
		private var _isCustomSize:Boolean;
		public function Source(value:String = null,w:int =0, h:int=0, isCustomSize:Boolean=false) {
			if (value) {
				_width = w;
				_height = h;
				_isCustomSize = isCustomSize;
				url = value
			}
		}		
		public function get url():String { return _url; }
		
		public function set url(value:String):void {
			_url = value;
			_loader = new Loader();
			_loader.contentLoaderInfo.addEventListener(Event.COMPLETE, onLoaderComplete, false, 0, true);
			//var loaderContext:LoaderContext = new LoaderContext();
			//loaderContext.securityDomain = SecurityDomain.currentDomain;
			_loader.load(new URLRequest(_url));
		}
		
		
		public function get color():uint { return _color; }
		
		public function set color(value:uint):void {
			_isSetColor = true;
			_color = value;
			if(_displayObject){
				setDisplayObjectColor(_displayObject, _color);
			}
		}
		
		override public function get width():Number {
			if (numChildren > 0) {
				return getChildAt(0).width;
			}else {
				return _width;
			}
		}
		override public function set width(value:Number):void {
			if (numChildren > 0) {
				getChildAt(0).width = value;
			}else {
				_width = value;
			}
		}
		
		
		
		override public function get height():Number {
			if (numChildren > 0 && !_isCustomSize) {
				return getChildAt(0).height; 
			}else {
				return _height;
			}
		}
		
		override public function set height(value:Number):void {
			
			if (numChildren > 0 && !_isCustomSize) {
				getChildAt(0).height = value;
			}else {
				_height = value;
			}
		}
		
		/*public function get initParams():Object { return _initParams; }
		
		public function set initParams(value:Object):void {
			_initParams = value;
		}*/
		
		public function get content():Object {
			if(numChildren>0){
				return getChildAt(0);
			}else {
				return _content;
			}
		}
		
		private function onLoaderComplete(e:Event):void {
			var loaderInfo:LoaderInfo = e.target as LoaderInfo;
			_displayObject = loaderInfo.content;
			if (_isSetColor) {
				color = _color;
			}
			objectToObject(_content, _displayObject);
			if (!_isCustomSize) {
				if(_width>0){
					_displayObject.width = _width;
				}
				if(_height>0){
					_displayObject.height = _height;
				}
			}
			addChild(_displayObject);
			dispatchEvent(new Event(Event.COMPLETE));
		}
	}	
}
	