package com.ruochi.component.button{
	import flash.display.Sprite;
	import flash.display.MovieClip;
	import flash.events.MouseEvent;
	import flash.events.Event;	
	import flash.net.URLRequest;
	import flash.events.FocusEvent;
	import gs.TweenFilterLite;
	import gs.TweenLite;
	import com.ruochi.shape.Rect;
	import com.ruochi.shape.GradientRoundRect;
	import com.ruochi.utils.SafeLoader;
	import com.ruochi.utils.setCenter;
	public class IconGlowBtn extends Sprite {
		private var _enable:Boolean = true;
		private var _ldr:SafeLoader;
		private var _frame:int = 1;
		private var _clickArea:Rect = new Rect();
		private var _iconWrapper:Sprite = new Sprite();
		private var _iconUrl:String;
		private var _btnWidth:Number;
		private var _btnHeight:Number;
		private var _color:uint = 0xff0000;
		private var _glowColor:uint = 0x660000;
		private var _clickGlowColor:uint = 0xff9900;
		private var _iconMc:MovieClip;
		private var _glowBody:Sprite;
		public function IconGlowBtn(src:String, w:Number=50, h:Number = 50) { 
			_iconUrl = src;
			_btnWidth = w;
			_btnHeight = h;
			init();
		}
		private function init(e:Event=null) {
			_clickArea.buttonMode = true;
			_ldr = new SafeLoader();
			_ldr.contentLoaderInfo.addEventListener(Event.COMPLETE, onLoaded, false, 0, true);
			_ldr.load(new URLRequest(_iconUrl));
		}
		private function buildUI() {
			_clickArea.alpha = 0;
			_clickArea.width = _btnWidth;
			_clickArea.height = _btnHeight;
			addChild(_clickArea);
			addChild(_iconWrapper);
			addListener();
			setCenter(iconMc, _clickArea);
			dispatchEvent(new Event("onInit"));
		}
		private function addListener() {
			_clickArea.addEventListener(MouseEvent.MOUSE_OVER, onOverEffect, false, 0, true);
			_clickArea.addEventListener(FocusEvent.FOCUS_IN, onOverEffect, false, 0, true);
			_clickArea.addEventListener(FocusEvent.FOCUS_OUT, onOutEffect, false, 0, true);
			_clickArea.addEventListener(MouseEvent.MOUSE_OUT,onOutEffect,false,0,true);
			_clickArea.addEventListener(MouseEvent.CLICK,onClickEffect,false,0,true);
		}
		private function onLoaded(e:Event) {
			_iconMc = e.target.loader.content
			_iconWrapper.addChild(e.target.loader.content);
			_glowBody = _iconWrapper as MovieClip;
			buildUI();
		}
		public function onOverEffect(e:Event) {			
			if (enable != false) {
				TweenFilterLite.to(_glowBody, .5, { type:"Glow", color:_glowColor, alpha:1, blurX:2, blurY:2 , strength:3} );
				//TweenLite.to(_glowBody, .5, { tint:null } );		
			}
		}
		public function onOutEffect(e:Event) {			
			if (enable != false) {
				TweenFilterLite.to(_glowBody, .5, { type:"Glow", color:_glowColor, alpha:0, blurX:2, blurY:2 , strength:3} );
				//TweenLite.to(_glowBody, .5, { tint:null } );		
			}
		}
		private function onClickEffect(e:MouseEvent) {
			if (enable != false) {
				TweenFilterLite.to(_glowBody, .1, { type:"Glow", color:_clickGlowColor, alpha:1, blurX:2, blurY:2 , strength:3, overwrite:false} );
			}
		}
		public function set enable(en:Boolean) {
			if (en) {				
				TweenLite.to(this,.5,{alpha:1});
			} else {
				TweenLite.to(this,.5,{alpha:.2});
			}
			_enable = en;
			mouseEnabled = _enable;
			mouseChildren = _enable;
		}
		public function get enable():Boolean {
			return _enable;
		}
		public function get iconMc():MovieClip {
			return _iconMc;
		}
		public function set glowBody(mc:Sprite) {
			_glowBody = mc;
		}
		public function set glowColor(col:uint) {
			_glowColor = col;
		}
		public function get glowBody():Sprite {
			return _glowBody;
		}
	}
}