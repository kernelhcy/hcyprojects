package com.ruochi.component{
	import com.ruochi.net.CrossURLLoader;
	import com.ruochi.shape.RoundRect;
	import com.ruochi.events.Eventer;
	import com.ruochi.utils.fullDimension;
	import com.ruochi.utils.resizeBitmap;
	import fl.motion.easing.*;
	import com.ruochi.events.Eventer;
	import gs.TweenLite;
	import flash.display.Bitmap;
	import flash.display.DisplayObject;
	import flash.display.Sprite;
	import flash.display.Loader;
	import flash.events.Event;
	import flash.events.IOErrorEvent;
	import flash.events.TimerEvent;
	import flash.events.MouseEvent;
	import flash.net.URLRequest;	
	import flash.net.URLLoaderDataFormat;
	import flash.utils.Timer;
	import flash.utils.ByteArray;
	import flash.net.navigateToURL;
	public class ImageContainer extends Sprite {
		private var _loadId:int = 0;
		private var _focusId:int = -1;
		private var _imageWidth:Number =100;
		private var _imageHeight:Number =100;
		private var _autoPlayTime:Number = 8;
		private var _binaryLdr:CrossURLLoader = new CrossURLLoader();
		private var _transTimer:Timer;
		private var _heightQuality:Boolean;
		private var _imageBlendMode:String = "normal"
		private var _transDuration:Number = 1.5;
		private var _windowOpen:String = "_blank";
		private var _dataXml:XML = new XML();
		private var _imageIn:Function;
		private var _imageOut:Function;
		public function ImageContainer() {
			init();
		}
		private function init():void {
			_transTimer = new Timer(_autoPlayTime * 1000, 0);
			_transTimer.addEventListener(TimerEvent.TIMER, timerHandler);
			_binaryLdr.dataFormat = URLLoaderDataFormat.BINARY;
			_binaryLdr.addEventListener(Event.COMPLETE, onComplete,false,0,true);
			_binaryLdr.addEventListener(IOErrorEvent.IO_ERROR, onComplete, false, 0, true);			
		}
		private function loadImage():void {
			_binaryLdr.crossLoad(new URLRequest(_dataXml.channel.item[_loadId].image[0]));
			dispatchEvent(new Eventer(Eventer.START));
		}
		private function onComplete(e:Event):void {
			var imageLdr = new Loader();
			imageLdr.contentLoaderInfo.addEventListener(Event.INIT, imageInit,false,0,true);
			var imageData:ByteArray = e.target.data as ByteArray;
			imageLdr.loadBytes(imageData);
		}
		private function imageInit(e:Event):void {
			var image:Sprite= new Sprite();
			var bitmap:Bitmap = e.target.loader.content as Bitmap;
			bitmap.smoothing = true;
			fullDimension(bitmap, _imageWidth, _imageHeight);
			if (bitmap.scaleX<.5&&_heightQuality==true) {
				bitmap.bitmapData = resizeBitmap(bitmap.bitmapData, bitmap.width, bitmap.height);
				fullDimension(bitmap,_imageWidth,_imageHeight);
			}
			image.addChild(bitmap);
			image.visible = false;
			image.alpha = 0;
			image.blendMode = _imageBlendMode;
			addChild(image);
			if (_dataXml.channel.item[_loadId].link[0] != undefined) {
				image.buttonMode = true;
				image.addEventListener(MouseEvent.CLICK, onClick, false, 0, true);
			}
			if (_loadId==0) {
				goto(0);
				_transTimer.start();
			}
			_loadId++;
			dispatchEvent(new Eventer(Eventer.COMPLETE));
			if (_loadId< _dataXml.channel.item.length()) {
				loadImage();
			}
		}
		private function timerHandler(e:Event) {			
			goto((_focusId + 1) % numChildren);
		}
		private function imageIn(id) {
			TweenLite.to(getChildAt(id),_transDuration,{autoAlpha:1});
		}
		private function imageOut(id) {
			TweenLite.to(getChildAt(id),_transDuration,{autoAlpha:0});
		}
		private function onClick(e:MouseEvent) {
			navigateToURL(new URLRequest(_dataXml.channel.item[getChildIndex(e.currentTarget as DisplayObject)].link[0]),_windowOpen);
		}
		public function run():void {			
			loadImage();
		}
		public function goto(id) {
			if (id != _focusId && id< numChildren) {
				if(_focusId>-1){
					imageOut(_focusId);
				}
				_focusId = id
				imageIn(_focusId);
				_transTimer.reset();
				_transTimer.start();
				dispatchEvent(new Eventer(Eventer.CHANGE,_focusId));
			}
		}
		/*public function set imageWidth(w:Number):void {
			_imageWidth = w;
		}
		public function set imageHeight(h:Number):void {
			_imageHeight = h;
		}*/
		public function get imageWidth():Number {
			return _imageWidth;
		}
		public function get imageHeight():Number {
			return _imageHeight;
		}
		public function set autoPlayTime(num:Number):void {
			_autoPlayTime = num;
		}
		public function set heightQuality(boo:Boolean):void {
			_heightQuality = boo;
		}
		public function set dataXml(xml:XML):void {
			_dataXml = xml;
		}
		public function set transDuration(num:Number):void {
			_transDuration = num;
		}
		public function set imageBlendMode(str:String):void {
			_imageBlendMode = str;
		}
		public function get focusId():int {
			return _focusId;
		}
		public function set windowOpen(s:String):void {
			_windowOpen = s;
		}
		public function set imageBlendMode(s:String):void {
			_imageBlendMode = s;
		}
	}
}