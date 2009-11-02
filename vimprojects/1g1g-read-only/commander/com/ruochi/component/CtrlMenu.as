package com.ruochi.component{
	import component.CircleBtnBase;
	import flash.display.Sprite;
	import gs.TweenLite;
	public class CtrlMenu extends Sprite {
		public static instance:CtrlMenu = new CtrlMenu();
		public function CtrlMenu() {
			init()
		}
		private function init() {
			buildUI();
			menuIn();
		}
		private function buildUI() {
			for (var i:int = 0; i < 6; i++) {
				var btn:CircleBtnBase = new CircleBtnBase() 
				btn.x = 80 * Math.cos(i * Math.PI / 3);
				btn.y =	80 * Math.sin(i * Math.PI / 3);
				btn.visible = false;
				btn.alpha = 0;
				btn.initBase()
				addChild(btn);
			}
		}
		public function menuIn():void {
			for (var i:int = 0; i < 6; i++) {
				var btn = getChildAt(i)
				TweenLite.to(btn, .5, { autoAlpha:1, delay:i*.1 } );
			}
		}
		public function menuOut():void {
			for (var i:int = 0; i < 6; i++) {
				var btn = getChildAt(i)
				TweenLite.to(btn, .5, { autoAlpha:0 } );
			}
		}
	}
}