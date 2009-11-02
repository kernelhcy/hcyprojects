package com.ruochi.component{
	import gs.*;
	import com.ruochi.component.*;
	public class GlowCircleBtn extends GlowBtnBase {
		public var color:uint = 0xff0000;
		public function GlowCircleBtn() {
			padding = new Array(1, 1, 1, 1);
			super();
		}
		public override function buildUI() {			
			bg = new Circle();
			shadow = new Circle();
			gradient = new GradientCircle();
			bg.color = color;
			shadow.color = color;
			b.alpha = 0;
			b.width = btnWidth;
			b.height = btnHeight;
			shadow.width = btnWidth;
			shadow.height = btnHeight;
			bg.width = btnWidth;
			bg.height = btnHeight;
			gradient.width = btnWidth;
			gradient.height = btnHeight;
			gradient.alpha = .4;

			styleText.x = padding[3];
			styleText.y = padding[0];
			iconSp.x = padding[3];
			iconSp.y = padding[0];
			addChild(shadow);
			addChild(bg);
			addChild(gradient);			
			if(iconUrl!=""){
				addChild(iconSp);
			}
			if(styleText.text!=""){
				addChild(styleText);
			}
			addChild(b);
			//TweenLite.to(bg, .5, { alpha:.3 } );
			TweenFilterLite.to(shadow, 0.2, { type:"Glow", color:shadowColor, alpha:.2, blurX:2, blurY:2, strength:3 });
			
			addListener();
		}
	}
}