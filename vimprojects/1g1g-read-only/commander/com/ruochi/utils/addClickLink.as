package com.ruochi.utils {
	import com.ruochi.common.CommonText;
	import flash.display.Sprite;
	import flash.net.navigateToURL;
	import flash.net.URLRequest;
	import flash.events.MouseEvent;
	public function addClickLink(sprite:Sprite, url:String, windowOpen:String = null):void {
		if (!windowOpen) {
			windowOpen = CommonText.BLANK;
		}
		sprite.addEventListener(MouseEvent.CLICK, function onSpriteClick(e:MouseEvent):void {
			navigateToURL(new URLRequest(url), windowOpen); trace(url,windowOpen,'-----------------------------');
		}, false, 0, true);
		sprite.buttonMode = true;
		
	}	
}