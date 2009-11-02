package com.ruochi.component {
	import com.ruochi.shape.RoundRect;
	import flash.display.DisplayObject;
	import flash.display.Sprite;
	import com.ruochi.text.StyleText;
	import com.ruochi.shape.Rect;
	import flash.events.MouseEvent;
	import flash.events.KeyboardEvent;
	import flash.events.Event;
	import com.ruochi.tween.Motion;
	import com.ruochi.utils.DelayCall;
	import com.ruochi.utils.deleteAll;
	import com.ruochi.display.removeDisplayObject;
	import com.ruochi.display.ElementContainer;
	import com.ruochi.geom.Color;
	public class TextItemList extends ElementContainer {
		private var _corner:int = 3;
		private var _bg:RoundRect = new RoundRect(200, 24, _corner, 0xcccccc);
		private var _wrapper:Sprite = new Sprite();
		private var _mask:RoundRect = new RoundRect(300, 100, _corner-1);
		private var _array:Array = new Array();
		private var _gapY:int = 0;
		private var _activeId:int =-1;
		private var _activeBgColor:uint = 0xff9900;
		private var _activeTextColor:uint = 0xffffff;
		private var _textItemBgColor:uint = 0xffffff;
		private var _textItemTextColor:uint = 0x666666;
		private var _duration:Number = 0.2;
		private var _isActive:Boolean;
		private var _borderWidth:int = 1;
		private var _autoSize:Boolean = false;
		private var _maxWidth:int;
		private var _delayCall:DelayCall;
		public function TextItemList() {
			init();
		}
		
		private function init():void {
			setChildren();
			addChildren();
			addListener();
		}
		
		
		private function setChildren():void{
			_wrapper.mask = _mask;
			_wrapper.x = _borderWidth
			_wrapper.y = _borderWidth;
			_mask.x = _borderWidth;
			_mask.y = _borderWidth;
		}
		
		private function addChildren():void{
			addChild(_bg);
			addChild(_wrapper);
			addChild(_mask);
		}
		
		private function addListener():void {
			addEventListener(MouseEvent.CLICK, onClick, false, 0, true);
			_wrapper.addEventListener(MouseEvent.MOUSE_OVER, onRollOver, false, 0, true);
			if (stage) {
				addStageListener();
			}else {
				addEventListener(Event.ADDED_TO_STAGE, onAddToStage);
			}
		}
		
		private function onRemovedFromStage(e:Event):void {
			close();
		}
		
		private function onAddToStage(e:Event):void {
			removeEventListener(Event.ADDED_TO_STAGE, onAddToStage);
			addStageListener();
		}
		
		private function addStageListener():void {
			stage.addEventListener(KeyboardEvent.KEY_DOWN, onKeyDown, false, 0, true);
		}
		private function onRollOver(e:MouseEvent):void {
			var id:int = _wrapper.getChildIndex(e.target as DisplayObject);
			activeId = id;
		}
		
		private function onKeyDown(e:KeyboardEvent):void {
			if(_isActive){
				if (e.keyCode == 40) {
					activeId = (activeId + 1) % _wrapper.numChildren;
				}else if(e.keyCode == 38) {
					activeId = (activeId + _wrapper.numChildren - 1) % _wrapper.numChildren;
				}else if (e.keyCode == 13) {
					if(_activeId>-1){
						callFunction();
					}else {
						close();
					}
				}else if (e.keyCode == 27) {
					close();
				}
			}
		}
		
		private function onClick(e:MouseEvent):void {
			callFunction();
		}
		
		private function callFunction():void {
			var textItemPackage:TextItemPackage = _array[_activeId] as TextItemPackage;
			textItemPackage.clickFunciton.apply(this, textItemPackage.clickFuncitonParams);
			close();
		}
		
		public function clear():void {
			if(_isActive){
				//close();
			}
			close();
			deleteAll(_wrapper);
			_array = new Array();
			_maxWidth = 0;
		}
		
		
		override public function get width():Number { return _width; }
		
		override public function set width(value:Number):void { 
			
			_width = value;
			var i:int;
			var textItem:TextItemWithTopLine;
			var length:int = _wrapper.numChildren;
			for (i = 0; i < length; i++) {
				textItem = _wrapper.getChildAt(i) as TextItemWithTopLine;
				textItem.width = _width - _borderWidth*2;
			}
			_mask.width = _width - _borderWidth * 2;
			_bg.width = _width;
		}
		
		public function set activeId(value:int):void {
			var textItemPackage:TextItemPackage;
			textItemPackage = _array[_activeId] as TextItemPackage;
			if (textItemPackage) {
				Motion.to(textItemPackage.textItem.bg, { color:_textItemBgColor }, _duration);
				Motion.to(textItemPackage.textItem.styleText, { color:_textItemTextColor }, 0);
			}
			_activeId = value;
			textItemPackage = _array[_activeId] as TextItemPackage;
			if (textItemPackage) {
				Motion.to(textItemPackage.textItem.bg, { color:_activeBgColor }, _duration);
				Motion.to(textItemPackage.textItem.styleText, { color:_activeTextColor }, 0);
			}			
		}
		
		
		public function get activeId():int { return _activeId; }
		
		public function get bg():RoundRect { return _bg; }
		
		public function get activeBgColor():uint { return _activeBgColor; }
		
		public function get activeTextColor():uint { return _activeTextColor; }
		
		public function get textItemBgColor():uint { return _textItemBgColor; }
		
		public function get textItemTextColor():uint { return _textItemTextColor; }
		
		public function set activeBgColor(value:uint):void {
			_activeBgColor = value;
		}
		
	
		
		public function set textItemBgColor(value:uint):void {
			_textItemBgColor = value;
		}
		
		public function set textItemTextColor(value:uint):void {
			_textItemTextColor = value;
		}
		
		public function set activeTextColor(value:uint):void {
			_activeTextColor = value;
		}
		
		public function get autoSize():Boolean { return _autoSize; }
		
		public function set autoSize(value:Boolean):void {
			_autoSize = value;
		}
		
		public function get array():Array { return _array; }
		
		public function get isActive():Boolean { return _isActive; }
	
	
		
		public function addItem(txt:String, func:Function = null, funcParams:Array = null):TextItemWithTopLine {
			var textItem:TextItemWithTopLine = new TextItemWithTopLine();
			var textItemPackage:TextItemPackage = new TextItemPackage();
			textItemPackage.textItem = textItem;
			textItemPackage.clickFunciton = func;
			textItemPackage.clickFuncitonParams = funcParams;
			textItemPackage.text = txt;
			textItem.styleText.text = txt;
			textItem.styleText.color = _textItemTextColor;
			textItem.topLine.color = _bg.color;
			textItem.description.color = Color.sum(Color.multiplication(textItemBgColor,.6),Color.multiplication(textItemTextColor,.4))
			_maxWidth = Math.max(_maxWidth, textItem.styleText.width);
			_array.push(textItemPackage);
			return textItem;
		}
		public function open():void {
			_isActive = true;
			if(_delayCall){
				_delayCall.stop();
			}
			_activeId = -1;
			var i:int;
			var length:int = _array.length;
			var gapY:int = 0;
			if(length>0){
				var textItem:TextItem;
				var textItemPackage:TextItemPackage;
				var nextY:int = 0;
				for (i = 0; i < length; i++) {
					textItemPackage = _array[i];
					if(i==0){
						(textItemPackage.textItem as TextItemWithTopLine).isShowTopLine = false;
					}
					//textItemPackage.textItem.styleText.color = _textItemTextColor;
					_wrapper.addChild(textItemPackage.textItem);
					/*if ((textItemPackage.textItem as TextItemWithTopLine).isShowTopLine && i>0) {
						gapY = 1;
					}else {
						gapY = 0;
					}*/
					textItemPackage.textItem.y = nextY + gapY;
					nextY += textItemPackage.textItem.height+gapY;
				}
				_bg.height = _wrapper.height + _borderWidth * 2;
				_mask.height = _wrapper.height;				
				if (_autoSize) {
					width = _maxWidth + 10;
				}
				DelayCall.call(.01, configStageListener); 
				//configStageListener();
				//stage.addEventListener(MouseEvent.MOUSE_DOWN, onStageMouseDown);
				_bg.visible = true;
			}			
			addEventListener(Event.REMOVED_FROM_STAGE, onRemovedFromStage, false, 0, true);
		}
		
		private function configStageListener():void {
			if(stage){
				stage.addEventListener(MouseEvent.MOUSE_DOWN, onStageMouseDown);
				stage.addEventListener(Event.RESIZE, onStageResize);
			}
		}
		
		private function onStageResize(e:Event):void {
			if (stage) {
				close();
			}
		}
		
		private function onStageMouseDown(e:MouseEvent):void {
			if(stage){
				if (!hitTestPoint(stage.mouseX, stage.mouseY)) {
					close();
				}
			}
		}
		public function close():void { 
			removeEventListener(Event.REMOVED_FROM_STAGE, onRemovedFromStage);
			if(stage){
				stage.removeEventListener(MouseEvent.MOUSE_DOWN, onStageMouseDown);
				stage.removeEventListener(Event.RESIZE, onStageResize);
			}
			_activeId = -1;
			_bg.visible = false;
			removeDisplayObject(this);
			_delayCall = DelayCall.call(.2, setIsActive, [false]);
		}
		public function removeItemAt(id:int):void {
			
		}
		
		private function setIsActive(value:Boolean):void {
			_isActive = value;
		}
	}
}
import com.ruochi.component.TextItem;
class TextItemPackage extends Object {
	public var text:String;
	public var textItem:TextItem;
	public var clickFunciton:Function;
	public var clickFuncitonParams:Array;
	public function TextItemPackage() {
		
	}
}