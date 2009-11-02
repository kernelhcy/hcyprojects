package com.ruochi.component{
	import flash.display.*
	import flash.events.*
	import flash.geom.*
	import flash.text.*
	import flash.net.*
	import gs.*
	import com.ruochi.utils.*
	import com.ruochi.component.Tip	
	public class Btn extends Sprite{
		public var tip,source:String;
		public var padding:Number;
		public var buttonEnable:Boolean;
		public var iconMc, bg, b:Sprite;
		public var ldr:Loader;
		public var frame:int;
		public function Btn(_source:String=null,_padding=0){
			var ldr = new Loader();
			source = _source;
			padding = _padding;
			init();
		}
		public function init(){
			buttonEnable = true;
			frame = 1;
			b = new Sprite();			
            b.graphics.beginFill(0x000000);
            b.graphics.drawRect(0, 0, 2, 2);
            b.graphics.endFill();
			b.alpha=0			
			bg = new Sprite();
            bg.graphics.beginFill(0x000000);
            bg.graphics.drawRect(0, 0, 2, 2);
            bg.graphics.endFill();
			bg.alpha = 0;
			
			iconMc = new Sprite();
			
			ldr = new Loader();
			ldr.contentLoaderInfo.addEventListener(Event.COMPLETE, onLoaded, false, 0, true)
			ldr.load(new URLRequest(source));			
			
			b.addEventListener(MouseEvent.MOUSE_OVER, onOverEffect, false, 0, true);
			b.addEventListener(FocusEvent.FOCUS_IN, onOverEffect, false, 0, true);
			b.addEventListener(FocusEvent.FOCUS_OUT, onOutEffect, false, 0, true);
			b.addEventListener(MouseEvent.MOUSE_OUT,onOutEffect,false,0,true);
			b.addEventListener(MouseEvent.CLICK,onClickEffect,false,0,true);
			TweenFilterLite.to(iconMc, 0, {type:"Glow", color:0x000000,  blurX:2, blurY:2, strength:3});
			b.buttonMode=true;	
			
			addChild(bg);
			addChild(iconMc);
			addChild(b);
			
		}
		private function onLoaded(e:Event) {
			e.target.loader.content.x=padding
			e.target.loader.content.y=padding
			iconMc.addChild(e.target.loader.content)
			b.width=e.target.loader.content.width+padding*2
			b.height = e.target.loader.content.height + padding * 2
			if (iconMc.getChildAt(0) is MovieClip) {
				iconMc.getChildAt(0).gotoAndStop(frame)
			}
			
		}
		private function onOverEffect(e:Event) {
			if (enable!=false) {
				TweenFilterLite.to(e.currentTarget.parent["iconMc"], .5, {type:"Glow", color:0xff4400, alpha:.8, blurX:2, blurY:2, strength:3});
				
			}
		}
		private function onOutEffect(e:Event) {
			if (enable!=false) {
				TweenFilterLite.to(iconMc, .5, {type:"Glow", color:0x0066ff, alpha:.2, blurX:1, blurY:1, strength:3});
				
			}
		}
		private static function onClickEffect(e:MouseEvent) {
			
		}
		public function set enable(_enable:Boolean) {
			if(_enable){
				b.mouseEnabled=true
				TweenLite.to(this,.5,{alpha:1})
				buttonEnable=_enable
			}else{
				b.mouseEnabled=false
				TweenLite.to(this,.5,{alpha:.2})
				buttonEnable=_enable
			}
		}		
		public function get enable():Boolean {
			return buttonEnable;
		}
	}
}