package com.ruochi.component {
	import flash.display.Sprite;
	import com.ruochi.text.StyleText;
	import com.ruochi.shape.Rect;
	import com.ruochi.layout.LayoutUtils;
	public class TextItemWithTopLine extends TextItem {
		private var _isShowTopLine:Boolean = false;
		private var _description:StyleText = new StyleText();
		private var _descriptionText:String;
		protected var _topLine:Rect = new Rect(100, 1);
		public function TextItemWithTopLine() {
			super();
			init();
		}
		private function init():void {
			setChildren();
			addChildren();
		}
		
		private function addChildren():void{
			addChild(_topLine);
			addChild(_description);
		}
		private function setChildren():void {
			isShowTopLine = _isShowTopLine;
			//_description.text = "adfasdfasdfasdf";
			_description.y = _styleText.y;
			_description.mouseEnabled = false;
			_topLine.alpha = .4;
			//_description.align = "right";
		}
		
		override public function set width(value:Number):void {
			super.width = value;
			LayoutUtils.horizontalStretch(_topLine, 5, 5);
			if(_descriptionText){
				_description.text = _descriptionText;
				LayoutUtils.setRight(_description, _margin.right.value);
				_description.visible = _description.x > LayoutUtils.getDisplayObjectRightX(_styleText);
			}
		}
		
		public function get isShowTopLine():Boolean { return _isShowTopLine; }
		
		public function set isShowTopLine(value:Boolean):void {
			_isShowTopLine = value;
			_topLine.visible = _isShowTopLine;
		}
		
		public function get description():StyleText { return _description; }
		
		public function get topLine():Rect { return _topLine; }
		
		public function get descriptionText():String { return _descriptionText; }
		
		public function set descriptionText(value:String):void {
			_descriptionText = value;
			_description.text = _descriptionText;
		}
	}
}