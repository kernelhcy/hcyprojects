package com.ruochi.component.trendChart {
	import flash.display.Sprite;
	import com.ruochi.shape.Circle;
	import com.ruochi.layout.setCenter;
	import flash.events.Event;
	import flash.events.MouseEvent;
	import flash.net.navigateToURL;
	import flash.net.URLRequest;
	import com.ruochi.component.trendChart.TrendChartConfig;
	import com.ruochi.component.Tip;
	import gs.TweenLite;
	public class PointUnit extends Sprite {
		private var _bgColor:uint = 0xffffff;
		private var _bodyCircle:Circle = new Circle(3,0xff6600);
		private var _bgCircle:Circle = new Circle(5, _bgColor);
		private var _clickArea:Circle = new Circle(10);
		private var _dataXml:XML;
		private static var _color:uint
		public function PointUnit() {
			
		}
		
		private function init():void{
			addChildren();			
			setChildren();
			configListener();
		}
		
		private function configListener():void{
			if (_dataXml.link[0]) {
				addEventListener(MouseEvent.CLICK, onClick, false, 0, true);
				buttonMode = true;
			}
			if (_dataXml.text[0]) {
				Tip.addTip(_dataXml.text[0], this);
			}
			_clickArea.addEventListener(MouseEvent.MOUSE_OVER, onClickAreaMouseOver, false, 0, true);
			_clickArea.addEventListener(MouseEvent.MOUSE_OUT, onClickAreaMouseOut, false, 0, true);
		}		
		
		private function onClickAreaMouseOut(e:MouseEvent):void {
			TweenLite.to(this, .5, { scaleX:1, scaleY:1 } );
		}
		
		private function onClickAreaMouseOver(e:MouseEvent):void {
			TweenLite.to(this, .5, { scaleX:2, scaleY:2 } );
		}
		private function addChildren():void {
			addChild(_bgCircle);
			addChild(_bodyCircle);
			addChild(_clickArea);
		}
		
		private function setChildren():void {
			_bodyCircle.color = _dataXml.color[0];
			setCenter(_clickArea);
			setCenter(_bgCircle);
			setCenter(_bodyCircle);
			_clickArea.alpha = 0;
			x = (_dataXml.xValue[0] - TrendChartConfig.xMin) / (TrendChartConfig.xMax - TrendChartConfig.xMin) * TrendChartConfig.chartWidth;
			y = TrendChartConfig.chartHeight - (_dataXml.yValue[0] - TrendChartConfig.yMin) / (TrendChartConfig.yMax - TrendChartConfig.yMin) * TrendChartConfig.chartHeight;
			if (String(_dataXml.visible[0]) == "false") {
				visible = false;
			}
		}
		public function set dataXml(xml:XML):void {
			_dataXml = xml;
			init();
		}
		private function onClick(e:MouseEvent):void {
			navigateToURL(new URLRequest(_dataXml.link[0]), "_self");
		}
	}
}