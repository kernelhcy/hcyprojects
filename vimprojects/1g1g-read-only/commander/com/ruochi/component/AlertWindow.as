package com.ruochi.component{
	import com.ruochi.events.Eventer;
	import flash.display.Sprite;
	import flash.display.Stage;
	import flash.events.Event;
	import com.ruochi.component.*
	import com.ruochi.shape.Rect;
	import flash.geom.Rectangle;
	import com.ruochi.utils.deleteAll;
	import gs.TweenLite;
	import com.ruochi.layout.setCenter;
	import flash.filters.GlowFilter;
	import com.ruochi.utils.graphic.setFilter;
	public class AlertWindow extends Sprite {
		private var _source:Sprite;
		private var _backGroundBg:Rect;
		private var _bg:RoundBg;
		private var _wrapper:Sprite = new Sprite();
		private var _shadowColor:uint;
		public static var instance:AlertWindow = new AlertWindow();
		public static var enable:Boolean = false;
		public function AlertWindow() {
			_backGroundBg = new Rect(100,100,0);
			_backGroundBg.alpha = 0;
			_backGroundBg.visible = false;
			addChild(_backGroundBg);		
			addChild(_wrapper);	
		}
		private function init():void {
			enable = true;
			_backGroundBg.width = stage.stageWidth
			_backGroundBg.height = stage.stageHeight;
			_bg = new RoundBg(_source.width, _source.height, 8, 1, 0xff0000, 0x990000);				
			_wrapper.addChild(_bg);
			_wrapper.addChild(_source);
			_wrapper.visible = true;
			setCenter(_wrapper, stage);			
			TweenLite.to(_backGroundBg, .5, { autoAlpha:0.7 } );
			dispatchEvent(new Event("open"));
		}
		public function close():void {
			_wrapper.visible = false;
			enable = false;
			TweenLite.to(_backGroundBg, .5, { autoAlpha:0, onComplete: onTweenEnd } );
			dispatchEvent(new Event("close"));
		}
		public function onTweenEnd():void {
			deleteAll(_wrapper);
		}
		public function set source(o:*):void {
			_source = o;
			init();
		}
		public function get source():* {
			return _source;
		}
		public static function newAlertWindow(src:*):void {
			if (!enable) {
				if (src is Sprite) {
					instance.source = src;
				}else if (src is String){
					instance.source = new TextAlert(src);
				}
			}
		}
		public static function removeAlertWindow(e:Event=null):void {
			if(enable){
				instance.close();
			}
		}	
		public function set shadowColor(col:uint):void {
			_shadowColor = col;
			setFilter(_bg, new GlowFilter(_shadowColor, 0x000000, 5, 5));
		}
	}
}
import flash.display.Sprite;
import com.ruochi.shape.Rect;
import com.ruochi.component.button.TextBtn;
import flash.events.Event;
import com.ruochi.text.StyleText;
import flash.events.MouseEvent;
import com.ruochi.component.AlertWindow;
class TextAlert extends Sprite {
	public var _styleText:StyleText = new StyleText();
	public var _bg:Rect; 
	public var _okBtn:TextBtn = new TextBtn("ok");
	public var _alertText:String;
	public function TextAlert(txt:String) {
		_alertText = txt;
		init();
	}
	private function init():void {
		buildUI();
		addListener();
	}
	private function buildUI():void {
		_styleText.x = 20;
		_styleText.y = 20;
		_styleText.text = _alertText;
		if (_styleText.width > 200) {
			_styleText.width = 200;		
			_styleText.wordWrap = true;
			_styleText.autoSize = "left";			
		}
		_styleText.align = "left";		
		_bg = new Rect(_styleText.width + 40, _styleText.height + 100);
		_bg.alpha = 0;					
		_okBtn.x = (_bg.width - _okBtn.width) / 2;
		_okBtn.y = _bg.height - 40;
		addChild(_bg);
		addChild(_styleText);
		addChild(_okBtn);
	}
	private function addListener():void{
		_okBtn.addEventListener(MouseEvent.CLICK,onClick,false,0,true)
	}
	private function onClick(e:MouseEvent):void {
		AlertWindow.removeAlertWindow();
	}
}