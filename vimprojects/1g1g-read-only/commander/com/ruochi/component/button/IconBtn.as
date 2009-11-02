package com.ruochi.component.button{
	import com.ruochi.component.button.BtnBase;
	import com.ruochi.display.Source;
	import com.ruochi.layout.LayoutUtils;
	import com.ruochi.layout.Margin;
	import flash.display.Sprite;
	import flash.display.MovieClip;
	import flash.events.Event;
	public class IconBtn extends BtnBase {
		private var _source:Source;
		private var _icon:String;
		//private var _iconSp:Source;
		private var _frame:int = 1;
		public function IconBtn(ico:String = "", w:Number = 14, h:Number = 14) {			
			super(w, h);
			_icon = ico;
			init();
		}
		public function init():void {
			initBase();			
			_source = new Source(_icon);
			_source.addEventListener(Event.COMPLETE, onComplete, false, 0, true);			
		}
		private function buildUI():void {
			LayoutUtils.place(_source, new Margin("auto"),bg);
			_source.mouseEnabled = false;
			addChild(_source);
		}
		public function onComplete(e:Event):void {
			buildUI();
			frame = _frame;
		}
		public function set frame(f:int):void {
			_frame = f;
			try {
				(_source.getChildAt(0)  as  MovieClip).gotoAndStop(_frame);
			} catch (e:Error) {

			}
		}
		public function get frame():int {
			return _frame;
		}
		/*public function get iconSp():SpriteLoader {
			return _iconSp;
		}
		public function set iconSp(sld:Source):void {
			_iconSp = sld;
		}*/
		public function get icon():String {
			return _icon;
		}
		
		public function get source():Source { return _source; }
	}
}