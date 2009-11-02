package com.ruochi.component{
	import com.ruochi.display.FrameSprite;
	import com.ruochi.shape.GradientRect;
	import com.ruochi.shape.RoundRect;
	import flash.display.DisplayObject;
	import flash.display.Shape;
	import flash.display.Sprite;
	import flash.display.MovieClip;
	import flash.events.MouseEvent;
	import flash.events.Event;
	import flash.events.FocusEvent;
	import com.ruochi.layout.LayoutUtils;
	import com.ruochi.layout.setCenter;
	import com.ruochi.tween.Motion;
	import com.ruochi.utils.setDisplayObjectColor;
	import flash.filters.GlowFilter;
	import com.ruochi.display.ElementContainer
	public class ShapeGlowBtn extends ElementContainer {
		private var _enable:Boolean = true;
		private var _clickArea:RoundRect = new RoundRect(16, 16,3, 0xff6600);
		private var _iconWrapper:FrameSprite = new FrameSprite();
		private var _gradientRect:GradientRect = new GradientRect();
		private var _glowColor:uint = 0xffffff;
		private var _glowAlpha:Number;
		private var _glowAlphaHover:Number = 1;
		private var _color1:uint;
		private var _color2:uint;
		private var _colorHover1:uint;
		private var _colorHover2:uint;
		public function ShapeGlowBtn(w:Number = 16,h:Number = 16, s:DisplayObject = null) {
			width = w;
			height = h;
			if (s) {
				icon = s;
			}
			init();
		}
		private function onOverEffect(e:Event):void  {
			Motion.to(this, { glowAlpha:_glowAlphaHover }, .4 );
			Motion.to(_gradientRect, { color1:_colorHover1, color2:_colorHover2 } );
		}
		private function onOutEffect(e:Event):void  {		
			Motion.to(this, { glowAlpha:0 }, .4  );
			Motion.to(_gradientRect, { color1:_color1, color2:_color2 } );
		}
		private function onClickEffect(e:MouseEvent) :void {
			if (_enable != false) {
				glowAlpha = .2;
				Motion.to(this, { glowAlpha:1 },.4 );
			}
		}
		private function init():void  {
			setChildren();
			addChildren();
			addListener();		
		}
		private function setChildren():void  {			
			buttonMode = true;			
			_clickArea.alpha = 0;
			mouseChildren = false;
			_gradientRect.mask = _iconWrapper;
			glowAlpha = 0;
		}
		private function addChildren():void {			
			addChild(_clickArea);
			addChild(_gradientRect);
			addChild(_iconWrapper);		
		}
		private function addListener():void  {
			addEventListener(MouseEvent.MOUSE_OVER, onOverEffect, false, 0, true);
			addEventListener(FocusEvent.FOCUS_IN, onOverEffect, false, 0, true);
			addEventListener(FocusEvent.FOCUS_OUT, onOutEffect, false, 0, true);
			addEventListener(MouseEvent.MOUSE_OUT, onOutEffect, false, 0, true);
			addEventListener(MouseEvent.MOUSE_DOWN, onClickEffect, false, 0, true);
		}
		public function set enable(value:Boolean):void  {
			_enable = value;
			if (_enable) {				
				Motion.to(this,{alpha:1});
			} else {
				Motion.to(this,{alpha:.2});
			}
			mouseEnabled = _enable;
			tabEnabled = _enable;
			mouseChildren = _enable;
		}
		
		override public function set width(value:Number):void {
			_width = value;
			_iconWrapper.width = _width;
			_clickArea.width = _width;
			_gradientRect.width = _width;
		}
		
		
		override public function set height(value:Number):void {
			_height = value;
			_iconWrapper.height = _height;
			_clickArea.height = _height;
			_gradientRect.height = _height;
		}
		
		public function get enable():Boolean {
			return _enable;
		}
		public function set icon(icon:DisplayObject):void {
			//setCenter(icon, this);
			
			_iconWrapper.addChild(icon);
			LayoutUtils.setCenter(icon);
			LayoutUtils.setMiddle(icon);
		}
		public function get icon():DisplayObject {
			return _iconWrapper.getChildAt(0);
		}
		
		public function set color(value:uint):void {
			color1 = value;
			color2 = value;
			
		}
		
		public function set colorHover(value:uint):void {
			_colorHover1 = value;
			_colorHover2 = value;
		}
		
		
		
		public function set bgColor(c:uint):void {
			_clickArea.color = c;
		}
		public function set glowAlpha(value:Number):void {
			_glowAlpha = value;
			filters = [new GlowFilter(_glowColor, _glowAlpha, 3, 3, 3, 2)];
		}
		
		public function get glowAlpha():Number { return _glowAlpha; }
		
		public function get glowColor():uint { return _glowColor; }
		
		public function set glowColor(value:uint):void {
			_glowColor = value;
		}
		
		public function get colorHover2():uint { return _colorHover2; }
		
		public function set colorHover2(value:uint):void {
			_colorHover2 = value;
		}
		
		public function get colorHover1():uint { return _colorHover1; }
		
		public function set colorHover1(value:uint):void {
			_colorHover1 = value;
		}
		
		public function get color2():uint { return _color2; }
		
		public function set color2(value:uint):void {
			_color2 = value;
			_gradientRect.color2 = _color2;
		}
		
		public function get color1():uint { return _color1; }
		
		public function set color1(value:uint):void {
			_color1 = value;
			_gradientRect.color1 = _color1;
		}
		
		public function get clickArea():RoundRect { return _clickArea; }
		
		public function get iconWrapper():FrameSprite { return _iconWrapper; }
		
		public function get glowAlphaHover():Number { return _glowAlphaHover; }
		
		public function set glowAlphaHover(value:Number):void 
		{
			_glowAlphaHover = value;
		}
	}
}