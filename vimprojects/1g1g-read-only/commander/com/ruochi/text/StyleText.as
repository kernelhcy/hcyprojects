package com.ruochi.text{
	import flash.text.TextField;
	import flash.text.TextFormat;
	import flash.text.AntiAliasType;
	import flash.text.TextFieldAutoSize;
	import flash.text.GridFitType;
	import flash.utils.Dictionary;
	import flash.text.Font;
	import flash.system.Capabilities;
	public class StyleText extends TextField {
		private static var _defaultFont:String;
		private static var _isEmbedFont:Boolean = false;
		private static var _topMargin:Number = 0;
		private static var _fontFamily:Array = new Array("华文黑体", "Microsoft YaHei", "微软雅黑", "verdana");
		private var _color:uint;
		private var _align:String;
		private var _textFormat:TextFormat = new TextFormat();
		public function StyleText() {
			super();
			y = 0;
			selectable = false;
			height = 1;
			wordWrap = false;
			autoSize = TextFieldAutoSize.LEFT;
			_textFormat.size = 12;
			_textFormat.font = defaultFont;
			//gridFitType = GridFitType.PIXEL
			embedFonts = _isEmbedFont;			
			draw();
		}
		
		override public function get y():Number { return super.y - _topMargin; }
		
		override public function set y(value:Number):void {
			super.y = value +_topMargin;
		}
		
		override public function get embedFonts():Boolean { return super.embedFonts; }
		
		override public function set embedFonts(value:Boolean):void {
			super.embedFonts = value;
			if (embedFonts) {
				antiAliasType = AntiAliasType.ADVANCED;
				//sharpness = 0;
				thickness = 100;
			}
		}
		
		public function draw():void {
			defaultTextFormat = _textFormat;
			setTextFormat(_textFormat);			
		}
		public function set color(value:uint):void {
			_color = value;
			_textFormat.color = _color;
			draw();
		}
		public function get color():uint {
			return _color;
		}
		public function set align(value:String):void {
			_align = value;
			autoSize = _align;
			_textFormat.align = _align;
			draw();
		}
		public function set size(value:Number):void{
			_textFormat.size = value;
			draw();
		}
		public function set font(value:String):void {
			_textFormat.font = value;
			draw();
		}
		public function set textFormat(tf:TextFormat):void {
			_textFormat = tf;
			draw()
		}
		public function get textFormat():TextFormat {
			return _textFormat;
		}
		public function set bold(value:Boolean):void {
			_textFormat.bold = value;
			draw();
		}
				
		override public function set alpha(value:Number):void {
			super.alpha = value;
			if (super.alpha < 1) {
				super.visible = false;
			}else {
				super.visible = true;
			}
		}
		
		static public function get defaultFont():String {
			if (_defaultFont == null) {
				var fontArray:Array = Font.enumerateFonts(true);
				var fontNameArray:Array = new Array();
				var fontName:String;
				for (var i:int = 0; i < fontArray.length; i++) {
					fontName = (fontArray[i] as Font).fontName;
					if (_fontFamily.indexOf(fontName) > -1) {
						fontNameArray.push(fontName);
					}
				}
				if (fontNameArray.indexOf("华文黑体") >-1 && Capabilities.os == "MacOS") {
					defaultFont = "华文黑体";
					topMargin = 5;
				}else if (fontNameArray.indexOf("Microsoft YaHei")>-1) {
					defaultFont = "Microsoft YaHei"; 
				}else if (fontNameArray.indexOf("微软雅黑")>-1) {
					defaultFont = "微软雅黑"; 
				}else {
					defaultFont = "Verdana";
				}
			}
			return _defaultFont; 
		}
		
		static public function set defaultFont(value:String):void {
			_defaultFont = value;
			
		}
		
		static public function get isEmbedFont():Boolean { return _isEmbedFont; }
		
		static public function set isEmbedFont(value:Boolean):void { ;
			_isEmbedFont = value;
		}
		
		static public function set topMargin(value:Number):void {
			_topMargin = value;
		}
		
		static public function get fontFamily():Array { return _fontFamily; }
		
		static public function set fontFamily(value:Array):void {
			_fontFamily = value;
		}
	}
}