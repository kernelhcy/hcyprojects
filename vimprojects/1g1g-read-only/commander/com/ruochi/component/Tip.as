package com.ruochi.component{
	import com.ruochi.shape.RectBorder;
	import com.ruochi.shape.RoundRect;
	import flash.display.Sprite;
	import flash.display.Stage;
	import flash.events.MouseEvent;
	import flash.events.Event;
	import gs.TweenFilterLite;
	import gs.TweenLite;
	import flash.utils.Dictionary;
	import com.ruochi.utils.deleteAll;
	import com.ruochi.text.StyleText;
	public class Tip extends Sprite {
		private var _tipText:String;
		private var _bg:RoundRect = new RoundRect(100, 100, 3, 0xffffff);
		private var _bgBorder:RoundRect = new RoundRect(100, 100, 5, 0xcccccc);
		private static var _tipTextColor:uint = 0x660000;
		private static var _tipMaxWidth:Number;
		private var _mouseGap:int = 20;
		private static var _stage:Stage;
		private var _styleText:StyleText = new StyleText();
		private static var tipDictionary:Dictionary = new Dictionary(true);
		public static var tip:Tip = new Tip();
		public function Tip() {
			addChild(_bgBorder);
			addChild(_bg);
			addChild(_styleText);
			visible = false;
		}
		private function setText(str:String):void {
			_styleText.multiline = false;
			_styleText.wordWrap = false;
			_styleText.width = 20;
			_styleText.text = str;
			_styleText.textColor = _tipTextColor;
			_styleText.x = 5;
			_styleText.y = 3;
			if (_styleText.width > _stage.stageWidth*.4) {				
				_styleText.multiline = true;
				_styleText.wordWrap = true;
				_styleText.autoSize = "left";
				_styleText.width = _stage.stageWidth*.4
			}
			_bg.width = _styleText.width + 6;
			_bg.height = _styleText.height + 2;
			_bg.x = 2;
			_bg.y = 2;
			_bgBorder.width = _bg.width + 4;
			_bgBorder.height = _bg.height + 4;
		}
		private function onMove(e:MouseEvent):void {
			setPosition()			
		}
		private function setPosition():void {
			if (_stage.mouseX + width + _mouseGap < _stage.stageWidth) {
				x = _stage.mouseX + _mouseGap;
			} else {
				x = _stage.mouseX - width - _mouseGap;
			}
			if (_stage.mouseY + height + _mouseGap < _stage.stageHeight) { 
				y = _stage.mouseY + _mouseGap;
			} else {
				y = _stage.mouseY - height - _mouseGap;
			}	
		}
		private static function tipIn(e:MouseEvent):void {
			_stage = e.currentTarget.stage; 
			_stage.addChild(tip);
			tip.add(e.currentTarget)
		}
		private static function tipOut(e:MouseEvent):void {
			TweenLite.killTweensOf(tip);
			e.currentTarget.removeEventListener(MouseEvent.MOUSE_MOVE, tip.onMove);
			tip.visible = false;
		}
		public function add(ob:Object=null):void{
			if(tipDictionary[ob]){
				setText(tipDictionary[ob]);
				setPosition();
				tip.visible = true;
				ob.addEventListener(MouseEvent.MOUSE_MOVE, onMove, false, 0, true);			
			}
		}
		public function set color(col:uint):void {
			_styleText.color = col;
		}
		public function set bgColor(col:uint):void {
			_bg.color = col;
		}
		public static function addTip(txt:String, spr:Sprite):void {
			tipDictionary[spr] = txt;
			spr.addEventListener(MouseEvent.MOUSE_OVER, tipIn, false, 0, true);
			spr.addEventListener(MouseEvent.MOUSE_OUT, tipOut, false, 0, true);
		}
		public static function removeTip(spr:Sprite):void {
			spr.removeEventListener(MouseEvent.MOUSE_OVER, tipIn);
			spr.removeEventListener(MouseEvent.MOUSE_OUT, tipOut);
			delete tipDictionary[spr];			
		}
		public static function set stage(sta:Stage):void {
			_stage = sta;
		}
	}
}