package com.ruochi.component{
	import flash.display.*;
	import flash.events.*;
	import flash.geom.*;
	import flash.text.*;
	import flash.net.*;
	import gs.*;
	import com.ruochi.utils.*;
	import com.ruochi.component.*;
	import com.ruochi.shape.Rect;
	import com.ruochi.shape.TabShape;
	public class TabBtn extends Sprite {
		public var padding:Array;
		public var _enable:Boolean = true;
		public var ldr:Loader;
		public var _frame:int = 1;
		public var b:Rect = new Rect();
		public var bg:TabShape = new TabShape() ;
		public var iconSp:Sprite = new Sprite();
		public var iconUrl:String;
		public var btnWidth:Number;
		public var btnHeight:Number;
		public var corner:Number = 3;
		public var styleText:StyleText = new StyleText();
		public var color:uint = 0xff0000;
		public var glowColor:uint = 0x660000;
		public var clickGlowColor:uint = 0xff9900;
		public function TabBtn() {
			this.addEventListener(Event.ADDED_TO_STAGE, init, false, 0, true);
		}
		public function init(e:Event=null) {
			b.buttonMode = true;			
		}
		private function buildUI() {
			bg = new TabShape();
			b.alpha = 0;
			b.width = btnWidth;
			b.height = btnHeight;
			bg.width = btnWidth;
			bg.height = btnHeight;
			bg.corner = corner;
			bg.color = color;
			styleText.x = padding[3];
			styleText.y = padding[0];
			iconSp.x = padding[3];
			iconSp.y = padding[0];
			addChild(bg);
			if(iconUrl!=""){
				addChild(iconSp);
			}
			if(styleText.text!=""){
				addChild(styleText);
			}
			addChild(b);
			addListener();			
			dispatchEvent(new Event("onInit"));
		}
		private function addListener() {
			b.addEventListener(MouseEvent.MOUSE_OVER, onOverEffect, false, 0, true);
			b.addEventListener(FocusEvent.FOCUS_IN, onOverEffect, false, 0, true);
			b.addEventListener(FocusEvent.FOCUS_OUT, onOutEffect, false, 0, true);
			b.addEventListener(MouseEvent.MOUSE_OUT,onOutEffect,false,0,true);
			b.addEventListener(MouseEvent.CLICK,onClickEffect,false,0,true);

		}
		private function onLoaded(e:Event) {
			iconSp.addChild(e.target.loader.content);			
			btnWidth = iconSp.width + padding[1] + padding[3];
			btnHeight = iconSp.height + padding[0] + padding[2];
			this.frame = _frame;
			buildUI();
		}
		private function onOverEffect(e:Event) {
		}
		private function onOutEffect(e:Event) {
		}
		private function onClickEffect(e:MouseEvent) {
			if (enable != false) {
				
			}
		}
		public function set enable(_enable:Boolean) {
			if (_enable) {
				b.mouseEnabled=true;
				TweenLite.to(this,.5,{alpha:1});
				_enable=_enable;
			} else {
				b.mouseEnabled=false;
				TweenLite.to(this,.5,{alpha:.2});
				_enable=_enable;
			}
		}
		public function get enable():Boolean {
			return _enable;
		}
		public function set btnText(te:String) {
			styleText.text = te;
			styleText.align = "center";
			styleText.width = styleText.textWidth + 4;
			styleText.height = styleText.textHeight + 4;
			if (padding == null) {
				padding = new Array(3, 8, 3, 8);
			}
			btnWidth = styleText.width + padding[1] + padding[3];
			btnHeight = styleText.height + padding[0] + padding[2];
			buildUI();
		}
		public function set icon(source:String) {
			iconUrl = source;
			ldr = new Loader();
			ldr.contentLoaderInfo.addEventListener(Event.COMPLETE, onLoaded, false, 0, true);
			ldr.load(new URLRequest(source));
			if (padding == null) {
				padding = new Array(0, 0, 0, 0);
			}
		}
		public function set frame(f:int) {
			try {
				(iconSp.getChildAt(0)  as  MovieClip).gotoAndStop(f);
			} catch (e:Error) {

			}
			_frame = f;
		}
		public function get frame():int {
			return _frame;
		}
	}
}