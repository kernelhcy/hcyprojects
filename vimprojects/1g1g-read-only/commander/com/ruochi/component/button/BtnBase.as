package com.ruochi.component.button{
	import com.ruochi.shape.RoundRect;
	import flash.display.Sprite;
	import flash.events.MouseEvent;
	import flash.events.Event;	
	import flash.events.FocusEvent;
	import flash.filters.GlowFilter;
	import com.ruochi.tween.Motion;
	import com.ruochi.shape.Rect;
	import com.ruochi.shape.GradientRoundRect;
	import com.ruochi.shape.RoundRect;
	import com.ruochi.utils.deleteAll;
	import com.ruochi.utils.setDisplayObjectColor;
	import com.ruochi.geom.Color;
	public class BtnBase extends Sprite {
		private var _enable:Boolean = true;
		private var _clickArea:Rect;
		private var _bg:RoundRect;
		private var _shadow:RoundRect;
		private var _gradient:GradientRoundRect;
		private var _hoverGradient:GradientRoundRect;
		protected var _btnWidth:Number = 64;
		protected var _btnHeight:Number = 20;
		private var _corner:Number = 3;
		private var _color1:uint = 0xe7ecf1;
		private var _color2:uint = 0xc9d5e3;
		private var _hoverColor1:uint = 0xffcc00;
		private var _hoverColor2:uint = 0xff3300;
		private var _clickGlowColor:uint = 0xff9900;
		private var _shadowColor:uint = 0x000000;
		private var _colorGlow:uint;
		private var _shadowAlpha:Number;
		private var _defaultShadowAlpha:Number = .4;
		public function BtnBase(w:Number, h:Number) {
			_btnWidth = w;
			_btnHeight = h;
		}
		public function initBase():void  {
			buttonMode = true;
			buildBase();
			addListenerBase();
		}
		public function buildBase():void {
			deleteAll(this);
			_clickArea = new Rect(_btnWidth, _btnHeight);
			_bg = new RoundRect(_btnWidth,_btnHeight,_corner,_color1);
			_shadow = new RoundRect(_btnWidth,_btnHeight,_corner);
			_gradient = new GradientRoundRect(_btnWidth, _btnHeight, _corner, _color1, _color2);
			_hoverGradient = new GradientRoundRect(_btnWidth, _btnHeight, _corner, _hoverColor1, _hoverColor2);
			_hoverGradient.alpha = 0;
			_clickArea.alpha = 0;
			colorGlow = _shadowColor;
			shadowAlpha = _defaultShadowAlpha;
			addChild(_shadow);
			addChild(_bg);
			addChild(_gradient);
			addChild(_hoverGradient);
			addChild(_clickArea);
		}
		private function addListenerBase():void {
			addEventListener(MouseEvent.MOUSE_OVER, onOverEffect, false, 0, true);
			addEventListener(FocusEvent.FOCUS_IN, onOverEffect, false, 0, true);
			addEventListener(FocusEvent.FOCUS_OUT, onOutEffect, false, 0, true);
			addEventListener(MouseEvent.MOUSE_OUT,onOutEffect,false,0,true);
			addEventListener(MouseEvent.CLICK,onClickEffect,false,0,true);
		}
		private function onOverEffect(e:Event):void {			
			if (_enable != false) {
				Motion.to(_hoverGradient ,{ alpha:1 }, .3 );
			}
		}
		private function onOutEffect(e:Event):void {			
			if (_enable != false) {
				Motion.to(_hoverGradient ,{ alpha:0 }, .3 );
			}
		}
		private function onClickEffect(e:MouseEvent):void {
			if (_enable != false) {
				colorGlow = _clickGlowColor;
				shadowAlpha = 1;
				Motion.to(this, { colorGlow:_shadowColor, shadowAlpha:_defaultShadowAlpha }, .3 );
			}
		}
		public function set enable(en:Boolean):void {
			if (en) {
				mouseEnabled = true;
				mouseChildren = true;
				Motion.to(this,{alpha:1});
				_enable=en;
			} else {
				mouseEnabled=false;
				mouseChildren = false;
				Motion.to(this,{alpha:.2});
				_enable=en;
			}
		}
		public function get enable():Boolean {
			return _enable;
		}
		public function get btnWidth():Number {
			return _btnWidth;
		}
		public function get btnHeight():Number {
			return _btnHeight;
		}
		public function get bg():RoundRect {
			return _bg;
		}
		public function set color1(value:uint):void {
			_color1 = value;
			_shadow.color = _color1;
			_bg.color = _color1
			_gradient.color1 = _color1;
		}
		public function set color2(value:uint):void {
			_color2 = value;
			color1 = Color.sum(Color.multiplication(value, .5) , Color.multiplication(_color1, .5));
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
		public function set shadowColor(value:uint):void {
			_shadowColor = value;
		}
		public function set colorGlow(value:uint):void {
			_colorGlow = value;
			setShadow();
		}
		public function set shadowAlpha(value:Number):void {
			_shadowAlpha = value;
			setShadow();
		}
		
		private function setShadow():void {
			_shadow.filters = [new GlowFilter(_colorGlow, _shadowAlpha, 2, 2)];
		}
		override public function set width(value:Number):void {
			_btnWidth = value;
			_clickArea.width = _btnWidth;
			_hoverGradient.width = _btnWidth;
			_bg.width = _btnWidth;
			_gradient.width = _btnWidth;
			_shadow.width = _btnWidth;
		}		
		public function get shadow():RoundRect { return _shadow; }
		
		public function get colorGlow():uint { return _colorGlow; }
		
		public function get shadowAlpha():Number { return _shadowAlpha; }
	}
}