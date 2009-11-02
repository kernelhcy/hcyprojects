package com.ruochi.component{
	import component.SoundController;
	import flash.display.*;
	import flash.events.*;
	import flash.geom.*;
	import flash.text.*;
	import flash.net.*;
	import gs.*;
	import com.ruochi.utils.*;
	import com.ruochi.component.*;
	public class GlowBtnBase extends Sprite {
		public var tip:Tip;
		public var tipText:String;
		public var padding:Array;
		public var _enable:Boolean = true;
		public var ldr:Loader;
		public var _frame:int = 1;
		public var b:Rect = new Rect();
		public var iconSp:Sprite = new Sprite();
		public var iconUrl:String;
		public var btnWidth:Number;
		public var btnHeight:Number;
		public var corner:Number = 3;
		public var styleText:StyleText = new StyleText();
		public var glowColor:uint = 0x660000;
		public var clickGlowColor:uint = 0xff9900;
		public var shadowColor:uint = 0x000000;
		public var bg:*;
		public var shadow:*;
		public var gradient:*;
		public function GlowBtnBase() {
			this.addEventListener(Event.ADDED_TO_STAGE, init, false, 0, true);
		}
		public function init(e:Event=null) {
			b.buttonMode = true;			
		}
		public function buildUI() {
			
		}
		internal function addListener() {
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
			if (enable != false) {
				TweenLite.to(bg, .3, {tint:0xff6600} );
				TweenLite.to(gradient, .3, {tint:0xff0000} );
				if (tipText!=null) {
					tip = new Tip(tipText, this.stage);
				}
			}
		}
		private function onOutEffect(e:Event) {
			if (enable != false) {
				TweenLite.to(bg, .5, { tint:null } );
				TweenLite.to(gradient, .3, {tint:null} );
				if (tip!=null) {
					tip.remove();
				}
			}
		}
		private function onClickEffect(e:MouseEvent) {
			if (enable != false) {
				TweenFilterLite.to(shadow, .1, { type:"Glow", color:clickGlowColor, alpha:1, blurX:2, blurY:2 , strength:3, overwrite:false} );
				TweenFilterLite.to(shadow, .3, { type:"Glow", color:shadowColor, alpha:.2, blurX:2, blurY:2,delay:.1 , strength:3 ,overwrite:false} );
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