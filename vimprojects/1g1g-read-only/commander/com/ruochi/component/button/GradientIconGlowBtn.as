package com.ruochi.component.button{
	import com.ruochi.component.SpriteLoader;
	import com.ruochi.shape.GradientRect;
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
	public class GradientIconGlowBtn extends Sprite {
		private var _gradient:GradientRect = new GradientRect();
		private var _hoverGradient:GradientRect = new GradientRect();
		private var _color1:uint;
		private var _color2:uint;
		private var _hoverColor1:uint;
		private var _hoverColor2:uint;
		private var _enable:Boolean = true;
		private var _ldr:SafeLoader;
		private var _frame:int = 1;
		private var _clickArea:Rect = new Rect();
		private var _iconWrapper:Sprite = new Sprite();
		private var _iconUrl:String;
		private var _btnWidth:Number;
		private var _btnHeight:Number;
		private var _glowColor:uint = 0x660000;
		private var _clickGlowColor:uint = 0xff9900;
		private var _iconMc:MovieClip;
		private var _hoverIconMc:MovieClip;
		public function GradientIconGlowBtn(src:String, w:Number = 50,h:Number = 50) {
			_iconUrl = src;
			_btnWidth = w;
			_btnHeight = h;
			init();
		}
		private function onOverEffect(e:Event) {			
			if (enable != false) {
				TweenLite.to(_hoverGradient, .5, { alpha:1 } );	
			}
		}
		private function onOutEffect(e:Event) {			
			if (enable != false) {
				TweenLite.to(_hoverGradient, .5, { alpha:0 } );
			}
		}
		private function init() {
			var _iconLdr = new SafeLoader();
			_iconLdr.contentLoaderInfo.addEventListener(Event.COMPLETE, onLoaded, false, 0, true);
			_iconLdr.load(new URLRequest(_iconUrl));
		}
		private function buildUI() {			
			_clickArea.buttonMode = true;
			_clickArea.alpha = 0;
			_clickArea.width = _btnWidth;
			_clickArea.height = _btnHeight;
			addChild(_iconWrapper);
			setCenter(_iconMc, _clickArea);
			_gradient.width = _iconMc.width;
			_gradient.height = _iconMc.height;
			_gradient.x = _iconMc.x;
			_gradient.y = _iconMc.y;
			_iconWrapper.addChild(_gradient);
			_iconWrapper.mask = _iconMc;
			_gradient.mouseEnabled = false;		
			_iconWrapper.addChild(_hoverGradient);
			_hoverGradient.alpha = 0;
			_hoverGradient.mouseEnabled = false;
			dispatchEvent(new Event("onInit"));
			addChild(_clickArea);
			addListener();			
			
		}
		private function addListener() {
			_clickArea.addEventListener(MouseEvent.MOUSE_OVER, onOverEffect, false, 0, true);
			_clickArea.addEventListener(FocusEvent.FOCUS_IN, onOverEffect, false, 0, true);
			_clickArea.addEventListener(FocusEvent.FOCUS_OUT, onOutEffect, false, 0, true);
			_clickArea.addEventListener(MouseEvent.MOUSE_OUT,onOutEffect,false,0,true);
			_clickArea.addEventListener(MouseEvent.CLICK,onClickEffect,false,0,true);
		}
		private function onHoverLoaded(e:Event) {
			_hoverIconMc = e.target.loader.content;
			_iconWrapper.addChild(_hoverIconMc);			
			//_hoverGradient.mask = _hoverIconMc;			
			_hoverGradient.width = _hoverIconMc.width;
			_hoverGradient.height = _hoverIconMc.height;
			_hoverGradient.x = _hoverIconMc.x;
			_hoverGradient.y = _hoverIconMc.y;
			setCenter(_hoverIconMc, _clickArea);
		}
		private function onLoaded(e:Event) {
			_iconMc = e.target.loader.content
			addChild(_iconMc);
			buildUI();			
			/*var _iconHoverLdr = new SafeLoader();
			_iconHoverLdr.contentLoaderInfo.addEventListener(Event.COMPLETE, onHoverLoaded, false, 0, true);
			_iconHoverLdr.load(new URLRequest(_iconUrl));*/
		}
		private function onClickEffect(e:MouseEvent) {
			if (enable != false) {
				TweenFilterLite.to(_hoverGradient, .1, { type:"Glow", color:_clickGlowColor, alpha:1, blurX:3, blurY:3 , strength:3, overwrite:false } );
				TweenFilterLite.to(_hoverGradient, .3, { type:"Glow", color:_clickGlowColor, alpha:0, blurX:2, blurY:2 , strength:3, delay:.1, overwrite:false} );
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
		public function set glowColor(col:uint) {
			_glowColor = col;
		}
		public function set color1(col:uint):void {
			_color1 = col;
			_gradient.color1 = _color1;
		}
		public function set color2(col:uint):void {
			_color2 = col;
			_gradient.color2 = _color2;
		}
		public function set hoverColor1(col:uint):void {
			_hoverColor1 = col;
			_hoverGradient.color1 = _hoverColor1;
		}
		public function set hoverColor2(col:uint):void {
			_hoverColor2 = col;
			_hoverGradient.color2 = _hoverColor2;
		}
		public function set clickGlowColor(col:uint):void {
			_clickGlowColor = col;
		}
	}
}