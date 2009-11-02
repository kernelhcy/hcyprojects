package com.ruochi.component{
	import com.ruochi.display.FrameSprite;
	import com.ruochi.shape.RoundRect;
	import flash.display.DisplayObject;
	import flash.display.Shape;
	import flash.display.Sprite;
	import flash.display.MovieClip;
	import flash.events.MouseEvent;
	import flash.events.Event;
	import flash.events.FocusEvent;
	import com.ruochi.layout.setCenter;
	import com.ruochi.tween.Motion;
	import com.ruochi.utils.setDisplayObjectColor;
	import flash.filters.GlowFilter;
	public class ShapeBtn extends Sprite {
		private var _enable:Boolean = true;
		private var _clickArea:RoundRect = new RoundRect(16, 16,3, 0xff6600);
		private var _iconWrapper:FrameSprite = new FrameSprite();
		private var _glowColor:uint = 0xffffff;
		private var _glowAlpha:Number;
		private var _width:int;
		private var _height:int;
		private var _iconColor:uint;
		public function ShapeBtn(w:Number = 16,h:Number = 16, s:DisplayObject = null) {
			_width = w;
			_height = h;
			_clickArea.width = _width;
			_clickArea.height = _height;
			if (s) {
				icon = s;
			}
			init();
		}
		private function onOverEffect(e:Event):void  {
			Motion.to(this, { glowAlpha:1 },.4 );
			//TweenFilterLite.to(_iconWrapper, 1, { glowFilter: { color:VcastrConfig.controlPanelBtnGlowColor, alpha:1, blurX:4, blurY:4, strength:3 }, ease:Elastic.easeOut,  overwrite:true } );
		}
		private function onOutEffect(e:Event):void  {		
			Motion.to(this, { glowAlpha:0 },.4  );
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
			glowAlpha = 0;
		}
		private function addChildren():void {			
			addChild(_clickArea);			
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
			mouseChildren = _enable;
		}
		public function get enable():Boolean {
			return _enable;
		}
		public function set icon(icon:DisplayObject):void {
			setCenter(icon, _clickArea)
			_iconWrapper.addChild(icon);
		}
		public function set iconColor(c:uint):void {
			setDisplayObjectColor(_iconWrapper, c);
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
	}
}