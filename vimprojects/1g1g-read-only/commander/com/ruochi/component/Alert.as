package component{
	import com.ruochi.component.button.TextBtn;
	import com.ruochi.shape.Rect;
	import com.ruochi.component.StyleText;
	import com.ruochi.utils.Eventer;
	import flash.events.MouseEvent;
	import flash.display.Sprite;
	import flash.events.Event;
	public class Alert extends Sprite {
		public var styleText:StyleText = new StyleText();
		public var bg:Rect = new Rect(240,240);
		public var okBtn:TextBtn = new TextBtn("确定");
		public var _alertText:String = "";
		public function Alert() {
			
		}
		private function init() {
			buildUI();
			addListener();
		}
		private function buildUI() {
			bg.alpha = 0;
			styleText.width = 200;
			styleText.height = 180;
			styleText.x = 20;
			styleText.y = 20;
			styleText.text = _alertText;
			styleText.align = "center"
			styleText.wordWrap = true;			
			okBtn.x = (240 - okBtn.width) / 2;
			okBtn.y = 200;			
			addChild(bg);
			addChild(styleText);
			addChild(okBtn);			
		}	
		private function addListener(){
			okBtn.b.addEventListener(MouseEvent.CLICK,onClick,false,0,true)
		}
		public function set alertText(str:String) {
			_alertText = str;
			init();
		}
		private function onClick(e:MouseEvent) {
			dispatchEvent(new Event("onClick"));
		}		
	}
}